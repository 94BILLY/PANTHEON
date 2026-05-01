/*===========================================================================
 * PANTHEON ENGINE — Path 1 (VU1) testbed
 *
 * EE packs 128-bit aligned DMA/VIF1 chains; VU1 (shader.vsm) transforms
 * vertices and kicks GIF. GIF path handles clear + libdraw overlays.
 * Softimage 3D → hrcConvert (NT) → ASCII .hrc → hrc2ps2.py → *_data.h
 *===========================================================================*/

#include <kernel.h>
#include <stdlib.h>
#include <malloc.h>
#include <tamtypes.h>
#include <math3d.h>
#include <packet.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gif_registers.h>
#include <gs_psm.h>
#include <dma.h>
#include <graph.h>
#include <graph_vram.h>
#include <draw.h>
#include <draw2d.h>
#include <draw3d.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "floor_data.h"
#include "skydome_data.h"
#include "pantheon_path1_contract.h"
#include "pantheon_timecycle.h"
#include "pantheon_vram.h"
#include "pantheon_texture_host.h"

#ifndef PANTHEON_TEXTURE_PHASE
#define PANTHEON_TEXTURE_PHASE 0
#endif

/*
 * Visual preset 1 = stable sandbox (flat quad / floor tile, ground snap, timecycle sky, Path 1).
 * Third-person follow by default; set PANTHEON_TRIAGE_FIRST_PERSON to 1 for optional FPV.
 */
#ifndef PANTHEON_VISUAL_PRESET
#define PANTHEON_VISUAL_PRESET 1
#endif

#ifndef PANTHEON_TRIAGE_DISABLE_SKY_PASS
#define PANTHEON_TRIAGE_DISABLE_SKY_PASS 0
#endif
#ifndef PANTHEON_TRIAGE_FIRST_PERSON
#define PANTHEON_TRIAGE_FIRST_PERSON 0
#endif

#ifndef PANTHEON_SKY_CLOUD_PUFF
/* Subtle GTA-style cloud puff into timecycle colors (vertex cost only; still Path 1). */
#define PANTHEON_SKY_CLOUD_PUFF 1
#endif

#ifndef PANTHEON_SKYDOME_PLAYER_Y_LIFT
/* Center authored skydome above feet so horizon rim meets atmosphere clear color. */
#define PANTHEON_SKYDOME_PLAYER_Y_LIFT 28.0f
#endif

#ifndef PANTHEON_ATMO_SMOOTH_ALPHA
/* Per-frame lerp toward target atmosphere (0..1). Higher = snappier; lower = smoother SA-style sky. */
#define PANTHEON_ATMO_SMOOTH_ALPHA 0.14f
#endif

#ifndef PANTHEON_CAM_DIST
/* Third-person follow distance tuned for grounded world-surface feel. */
#define PANTHEON_CAM_DIST 220.0f
#endif
#ifndef PANTHEON_CAMERA_HEIGHT_OFFSET
#define PANTHEON_CAMERA_HEIGHT_OFFSET 18.0f
#endif

/* Authored Softimage skydome in VU1 when cruncher produced tris (else CPU placeholder). */
#define USE_VU1_SKYDOME_MESH (SKYDOME_TRI_COUNT > 0)

#ifndef PANTHEON_FPV_EYE_HEIGHT
#define PANTHEON_FPV_EYE_HEIGHT 20.0f
#endif

// ==================== GTA SAN ANDREAS CAMERA ====================
static char padBuf[256] __attribute__((aligned(64)));

static float player_x = 0.0f, player_y = 0.0f, player_z = 0.0f;
static float player_yaw = 0.0f;
static float g_cam_dist = PANTHEON_CAM_DIST; /* L1 = zoom out, R1 = zoom in */

/* First-frame pose before update_camera_orbit (FPV: eye over origin; orbit: legacy offset). */
static VECTOR camera_position = {
    0.0f,
    PANTHEON_TRIAGE_FIRST_PERSON ? PANTHEON_FPV_EYE_HEIGHT : 62.76f,
    PANTHEON_TRIAGE_FIRST_PERSON ? 0.0f : 103.56f,
    1.0f};
static VECTOR camera_rotation = {
    PANTHEON_TRIAGE_FIRST_PERSON ? 0.0f : 0.55f,
    0.0f,
    0.0f,
    1.0f};

#ifndef PANTHEON_PAD_DEADZONE
#if PANTHEON_VISUAL_PRESET == 1
#define PANTHEON_PAD_DEADZONE 14.0f
#else
#define PANTHEON_PAD_DEADZONE 24.0f
#endif
#endif
static const float PAD_DEADZONE = PANTHEON_PAD_DEADZONE;
static const float RIGHT_YAW_SENS = 0.00135f;
static const float RIGHT_PITCH_SENS = 0.00095f;
static const float MOVE_SPEED = 2.15f;
static const float PI_F = 3.14159265f;

static float cam_yaw = 0.0f;
#if PANTHEON_VISUAL_PRESET == 1
static float cam_pitch = PANTHEON_TRIAGE_FIRST_PERSON ? 0.0f : 0.60f;
static float target_yaw = 0.0f;
static float target_pitch = PANTHEON_TRIAGE_FIRST_PERSON ? 0.0f : 0.60f;
#else
/* Default ~0.52 rad ≈ 30° down — GTA SA-style over-the-shoulder (tweak with right stick). */
static float cam_pitch = PANTHEON_TRIAGE_FIRST_PERSON ? 0.0f : 0.52f;
static float target_yaw = 0.5f;
static float target_pitch = PANTHEON_TRIAGE_FIRST_PERSON ? 0.0f : 0.52f;
#endif
static int pad_ready = 0;
/* Set if padPortOpen() succeeded. Pad may not be PAD_STATE_STABLE until after a few
 * full frames; a short boot-time busy-wait is not enough, so we finish init in the main loop. */
static int pad_port_open_ok = 0;
static int pad_mode_requested = 0;
static int analog_mode_active = 0;
// ============================================================

#define P_VIF_MPG    0x4a
#define P_VIF_UNPACK 0x6c
#define P_VIF_MSCAL  0x14
#define P_VIF_FLUSHE 0x10
#define P_VIF_ITOP   0x04

#define VIF_CODE(cmd, num, imm) \
    ((u32)(((u32)(cmd) << 24) | ((u32)(num) << 16) | (u32)(imm)))

extern u32 PantheonShader    __attribute__((section(".vutext")));
extern u32 PantheonShaderEnd __attribute__((section(".vutext")));

#define RENDER_STAGE 3
#define SHOW_CPU_PROBE 0
#define SHOW_CPU_EXPORTED_FLOOR 0
#define SHOW_SKY_PLACEHOLDER 1
/* Render profiles:
 * 0 = stable hybrid baseline (CPU overlay + Path1)
 * 1 = strict Path1 validation (no CPU overlay) */
#ifndef PANTHEON_RENDER_PROFILE
/* 0 = hybrid (CPU GIF floor + Path1): reliable floor during intro + matches buttery baseline. */
#define PANTHEON_RENDER_PROFILE 0
#endif
#if PANTHEON_RENDER_PROFILE == 1
#define PATH1_AB_CPU_OVERLAY 0
#else
#define PATH1_AB_CPU_OVERLAY 1
#endif

#ifndef PANTHEON_MIN_TELEMETRY
#define PANTHEON_MIN_TELEMETRY 0
#endif

#ifndef PANTHEON_TRIAGE_STATIC_CAMERA
/* 1 = fixed eye (whitebox). Default 0 = orbit follow — same matrices as movement (avoids view “exploding” on look). */
#define PANTHEON_TRIAGE_STATIC_CAMERA 0
#endif

#ifndef PANTHEON_TRIAGE_ENABLE_SKYDOME
#define PANTHEON_TRIAGE_ENABLE_SKYDOME 1
#endif

#ifndef PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER
/* 0 = world-anchored tiled floor (recommended). 1 = follow patch (can drift vs authored mesh span). */
#define PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER 0
#endif
#ifndef PANTHEON_TRIAGE_FORCE_FLAT_QUAD
#define PANTHEON_TRIAGE_FORCE_FLAT_QUAD 0
#endif
#ifndef PANTHEON_TRIAGE_LOCK_RIGHT_STICK
#define PANTHEON_TRIAGE_LOCK_RIGHT_STICK 0
#endif
#ifndef PANTHEON_TRIAGE_WORLDVIEW_YAW_GUARD
/* Must stay 0 for normal play: camera_position is offset by cam_yaw but create_world_view
 * must use the same rotation (see ps2sdk draw samples). Yaw-guard mis-binds view ↔ orbit. */
#define PANTHEON_TRIAGE_WORLDVIEW_YAW_GUARD 0
#endif
#ifndef PANTHEON_TRIAGE_PITCH_SIGN_FLIP
/* If floor looks like a distant wall, flip pitch sign for create_world_view convention. */
#define PANTHEON_TRIAGE_PITCH_SIGN_FLIP 0
#endif

#ifndef PANTHEON_CAMERA_ENVELOPE_HARDEN
#define PANTHEON_CAMERA_ENVELOPE_HARDEN 1
#endif

#ifndef PANTHEON_PLAYER_ABYSS_FALL_STEP
#define PANTHEON_PLAYER_ABYSS_FALL_STEP 0.55f
#endif
#ifndef PANTHEON_PLAYER_ABYSS_Y_MIN
#define PANTHEON_PLAYER_ABYSS_Y_MIN -4000.0f
#endif

#ifndef PANTHEON_VIEW_NEAR
#define PANTHEON_VIEW_NEAR 1.0f
#endif
/* Perspective window at the near plane (same units as cube.teapot samples: ±3).
 * Huge values (e.g. ±500) make the ground plane a razor-thin strip in NDC. */
#ifndef PANTHEON_VIEW_FRUSTUM_LR
#define PANTHEON_VIEW_FRUSTUM_LR 96.0f
#endif
#ifndef PANTHEON_VIEW_FRUSTUM_BT
#define PANTHEON_VIEW_FRUSTUM_BT 72.0f
#endif

#ifndef PANTHEON_VU_NEARZ
#define PANTHEON_VU_NEARZ 0.01f
#endif
#ifndef PANTHEON_PATH1_GIF_USE_ST
#define PANTHEON_PATH1_GIF_USE_ST 1
#endif
#ifndef PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
#define PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE 1
#endif

/* Duplicate each tri with reversed winding (2x verts). WARNING: coplanar opposite faces Z-fight
 * badly on some hosts (black / flashing in PCSX2 Vulkan). Default OFF; use 1 only if culling hides
 * the floor and you accept the artifact, or fix SoftImage winding instead. */
#ifndef PANTHEON_FLOOR_PATH1_DOUBLE_SIDED
#define PANTHEON_FLOOR_PATH1_DOUBLE_SIDED 0
#endif

#if PANTHEON_FLOOR_PATH1_DOUBLE_SIDED
#define PANTHEON_FLOOR_PATH1_VERTS (FLOOR_TRI_COUNT * 3 * 2)
#else
#define PANTHEON_FLOOR_PATH1_VERTS (FLOOR_TRI_COUNT * 3)
#endif

#ifndef PANTHEON_TILE_RADIUS
/* Tiles in ±radius around player (e.g. 2 => 5×5 SoftImage floor patches). */
#define PANTHEON_TILE_RADIUS 2
#endif

#ifndef PANTHEON_TILE_SUBMIT_BUDGET
#define PANTHEON_TILE_SUBMIT_BUDGET 25
#endif
#ifndef PANTHEON_FLOOR_FOLLOW_TILE_SIZE
#define PANTHEON_FLOOR_FOLLOW_TILE_SIZE 60.0f
#endif

#if (PANTHEON_TILE_SUBMIT_BUDGET <= 0)
#error "PANTHEON_TILE_SUBMIT_BUDGET must be > 0."
#endif

/* Must match the scale applied in init_flat_floor() to floor mesh (Softimage export). */
#define FLOOR_MESH_SCALE 14.0f

#if USE_VU1_SKYDOME_MESH
/* Scale Softimage mesh units (~radius 5 sphere) into world space. */
#define SKYDOME_MESH_SCALE 320.0f
static PantheonVertex flat_skydome[SKYDOME_TRI_COUNT * 3] __attribute__((aligned(16)));
#endif

packet_t *packets[2];
int context = 0;
static int g_vif_context = 0;
static int g_last_nearz_hits = 0;
static int g_last_batch_verts = 0;
static int g_last_vif1_qw = 0;
static int g_last_tile_submits = 0;
static const char *g_path1_mesh_stage = "unknown";
static float g_floor_center_x = 0.0f;
static float g_floor_center_z = 0.0f;
static PantheonAtmosphere g_atmosphere;
static PantheonAtmosphere g_atmosphere_target;
static int g_atmosphere_inited = 0;
/* Default: soft baby-blue overcast daylight (see pantheon_timecycle.h OVERCAST row). */
static PantheonWeather g_weather = PANTHEON_WEATHER_OVERCAST;
static float g_day01 = 0.20f;
/* Cross-fade between weather segment presets (avoids instant hue jumps every ~18 min). */
static int g_weather_blend_remaining = 0;
#define PANTHEON_WEATHER_BLEND_FRAMES 90
#ifndef PANTHEON_WEATHER_AUTO_CYCLE
#define PANTHEON_WEATHER_AUTO_CYCLE 0
#endif
#ifndef PANTHEON_WEATHER_SLOT_SECONDS
#define PANTHEON_WEATHER_SLOT_SECONDS 300
#endif
#ifndef PANTHEON_DAY_CYCLE_SECONDS
#define PANTHEON_DAY_CYCLE_SECONDS 1440.0f
#endif

typedef enum PantheonRenderJobType {
    PANTHEON_RENDER_JOB_CLEAR = 0,
    PANTHEON_RENDER_JOB_CPU_DEBUG,
    PANTHEON_RENDER_JOB_PATH1_SKY,
    PANTHEON_RENDER_JOB_PATH1_FLOOR
} PantheonRenderJobType;

typedef struct PantheonRenderJob {
    PantheonRenderJobType type;
    int enabled;
} PantheonRenderJob;

// Forward declarations
qword_t *render_clear_and_setup(qword_t *q, int ctx, framebuffer_t *frame, zbuffer_t *z, int frame_id);
qword_t *render_floor_path1_count(qword_t *q, MATRIX mvp, int total_verts);
static qword_t *render_path1_tris(qword_t *q, MATRIX mvp, const PantheonVertex *mesh_tris, int total_verts, int max_verts);
static void update_atmosphere(int frame_id);
static void update_skydome_colors(void);
#if !USE_VU1_SKYDOME_MESH
static qword_t *render_sky_placeholder_dome(qword_t *q, MATRIX world_view, MATRIX view_screen);
#endif
static void clamp_player_to_floor(void);
static void apply_sky_color(PantheonVertex *v);
static u8 pantheon_boot_reveal_luma(int frame_id);
static int pantheon_boot_reveal_active(int frame_id);
static qword_t *render_boot_title_overlay(qword_t *q, int frame_id);

#ifndef PANTHEON_BOOT_REVEAL_ENABLE
#define PANTHEON_BOOT_REVEAL_ENABLE 1
#endif

#ifndef PANTHEON_BOOT_REVEAL_STEP
#define PANTHEON_BOOT_REVEAL_STEP 2
#endif

#ifndef PANTHEON_BOOT_REVEAL_HOLD_FRAMES
#define PANTHEON_BOOT_REVEAL_HOLD_FRAMES 180
#endif

#define PANTHEON_BOOT_REVEAL_HALF_FRAMES (255 / PANTHEON_BOOT_REVEAL_STEP)
#define PANTHEON_BOOT_REVEAL_TOTAL_FRAMES ((PANTHEON_BOOT_REVEAL_HALF_FRAMES * 2) + PANTHEON_BOOT_REVEAL_HOLD_FRAMES)

#ifndef PANTHEON_BOOT_TITLE_SCALE
#define PANTHEON_BOOT_TITLE_SCALE 3
#endif

/* libdraw draw_rect_filled() adds START_OFFSET to v0 and END_OFFSET to v1 (see ps2sdk draw2d.c).
 * draw_clear(..., 2048-320, 2048-224, 640, 448) uses framebuffer origin in GS space at (1728,1824).
 * So pass vertex coords: v0 = fb_xy + origin - START_OFFSET, v1 = fb_xy+size + origin - END_OFFSET. */
#define PANTHEON_DRAW_GS_FB_X0 (2048.0f - 320.0f)
#define PANTHEON_DRAW_GS_FB_Y0 (2048.0f - 224.0f)
#define PANTHEON_LIBDRAW_START_OFFSET 2047.5625f
#define PANTHEON_LIBDRAW_END_OFFSET 2048.5625f

static float wrap_angle_pi(float v) {
    while (v > PI_F) v -= (2.0f * PI_F);
    while (v < -PI_F) v += (2.0f * PI_F);
    return v;
}

static void try_finish_pad_init(void) {
    if (pad_ready || !pad_port_open_ok)
        return;

    int st = padGetState(0, 0);
    if (st == PAD_STATE_DISCONN || st == PAD_STATE_ERROR)
        return;

    if (!pad_mode_requested) {
        if (st != PAD_STATE_STABLE)
            return;

        if (padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK) != 0) {
            pad_mode_requested = 1;
        }
        return;
    }

    if (padGetReqState(0, 0) != PAD_RSTAT_COMPLETE)
        return;

    if (!analog_mode_active) {
        int mode = padInfoMode(0, 0, PAD_MODECURID, 0);
        if (mode == PAD_TYPE_DUALSHOCK || mode == PAD_TYPE_ANALOG) {
            analog_mode_active = 1;
        } else {
            /* Keep retrying analog activation until the controller negotiates. */
            pad_mode_requested = 0;
            return;
        }
    }

    pad_ready = 1;
}

static void read_pad_analog(void) {
    if (!pad_ready) {
        return;
    }

    struct padButtonStatus buttons;
    buttons.ljoy_h = 128;
    buttons.ljoy_v = 128;
    buttons.rjoy_h = 128;
    buttons.rjoy_v = 128;

    if (padRead(0, 0, &buttons) == 0) {
        return;
    }

    float lx = (float)buttons.ljoy_h - 127.5f;
    float ly = (float)buttons.ljoy_v - 127.5f;
    float rx = (float)buttons.rjoy_h - 127.5f;
    float ry = (float)buttons.rjoy_v - 127.5f;

    if (fabsf(lx) < PAD_DEADZONE) {
        lx = 0.0f;
    }
    if (fabsf(ly) < PAD_DEADZONE) {
        ly = 0.0f;
    }
    if (fabsf(rx) < PAD_DEADZONE) {
        rx = 0.0f;
    }
    if (fabsf(ry) < PAD_DEADZONE) {
        ry = 0.0f;
    }
#if PANTHEON_TRIAGE_LOCK_RIGHT_STICK
    rx = 0.0f;
    ry = 0.0f;
#endif

    u32 paddata = 0xffffu ^ (u32)buttons.btns;

    /* L1/R1 = camera zoom (sandbox feel). */
    if (paddata & PAD_L1) {
        g_cam_dist = fminf(800.0f, g_cam_dist + 4.0f);
    }
    if (paddata & PAD_R1) {
        g_cam_dist = fmaxf(40.0f, g_cam_dist - 4.0f);
    }

    target_yaw += rx * RIGHT_YAW_SENS;
    target_pitch += ry * RIGHT_PITCH_SENS;
    target_yaw = wrap_angle_pi(target_yaw);

    /* Sandbox limits: horizon → near-straight-down. */
    if (target_pitch < 0.05f) {
        target_pitch = 0.05f;
    }
    if (target_pitch > 1.50f) {
        target_pitch = 1.50f;
    }

    cam_yaw += (target_yaw - cam_yaw) * 0.165f;
    cam_pitch += (target_pitch - cam_pitch) * 0.155f;
    cam_yaw = wrap_angle_pi(cam_yaw);
    if (cam_pitch < 0.05f) {
        cam_pitch = 0.05f;
    }
    if (cam_pitch > 1.50f) {
        cam_pitch = 1.50f;
    }

    /* Movement (camera-relative). */
    float mvx = lx;
    float mvy = ly;
    if (paddata & (PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN)) {
        mvx = ((paddata & PAD_RIGHT) ? 127.5f : 0.0f) - ((paddata & PAD_LEFT) ? 127.5f : 0.0f);
        mvy = ((paddata & PAD_UP) ? 127.5f : 0.0f) - ((paddata & PAD_DOWN) ? 127.5f : 0.0f);
    }

    /* DualShock: stick up is negative ly; flip Y so walk "forward"/"back" matches camera-relative math + D-pad. */
    mvy = -mvy;

    float mag = sqrtf(mvx * mvx + mvy * mvy);
    if (mag > 0.0f) {
        float nx = mvx / mag;
        float ny = mvy / mag;
        float move_scale = MOVE_SPEED * (mag / 127.5f);
        float move_x = (sinf(cam_yaw) * -ny) + (cosf(cam_yaw) * nx);
        float move_z = (cosf(cam_yaw) * -ny) - (sinf(cam_yaw) * nx);

        player_x += move_x * move_scale;
        player_z += move_z * move_scale;
        player_yaw = wrap_angle_pi(atan2f(nx, -ny) + cam_yaw);
    }

    clamp_player_to_floor();
}

#if !PANTHEON_TRIAGE_STATIC_CAMERA
static void update_camera_orbit(void) {
#if PANTHEON_TRIAGE_FIRST_PERSON
    camera_position[0] = player_x;
    camera_position[1] = player_y + PANTHEON_FPV_EYE_HEIGHT;
    camera_position[2] = player_z;
    camera_position[3] = 1.0f;
    camera_rotation[0] = PANTHEON_TRIAGE_PITCH_SIGN_FLIP ? cam_pitch : -cam_pitch;
    camera_rotation[1] = cam_yaw;
    camera_rotation[2] = 0.0f;
    camera_rotation[3] = 1.0f;
#else
    /* Third-person sandbox follow (GTA SA / MGS2-style testbed). */
    float cos_p = cosf(cam_pitch);
    float sin_p = sinf(cam_pitch);

    camera_position[0] = player_x + (sinf(cam_yaw) * cos_p * g_cam_dist);
    camera_position[1] = player_y + (sin_p * g_cam_dist) + PANTHEON_CAMERA_HEIGHT_OFFSET;
    camera_position[2] = player_z + (cosf(cam_yaw) * cos_p * g_cam_dist);
    camera_position[3] = 1.0f;

    camera_rotation[0] = PANTHEON_TRIAGE_PITCH_SIGN_FLIP ? cam_pitch : -cam_pitch;
    camera_rotation[1] = cam_yaw;
    camera_rotation[2] = 0.0f;
    camera_rotation[3] = 1.0f;
#endif
}
#endif

static void update_atmosphere(int frame_id) {
    /* Advance simulated day at a slower cadence to avoid visible clear-color stepping. */
    g_day01 += 1.0f / (60.0f * PANTHEON_DAY_CYCLE_SECONDS);
    if (g_day01 >= 1.0f) {
        g_day01 -= 1.0f;
    }

    PantheonWeather w_target = g_weather;
#if PANTHEON_WEATHER_AUTO_CYCLE
    /* Optional showcase mode: long-slot weather transitions. */
    int cycle = (frame_id / (60 * PANTHEON_WEATHER_SLOT_SECONDS)) & 3;
    w_target = (PantheonWeather)cycle;
#endif

    if (w_target != g_weather && g_weather_blend_remaining == 0) {
        g_weather_blend_remaining = PANTHEON_WEATHER_BLEND_FRAMES;
    }

    if (g_weather_blend_remaining > 0) {
        float bt = 1.0f - ((float)g_weather_blend_remaining / (float)PANTHEON_WEATHER_BLEND_FRAMES);
        PantheonAtmosphere a_from = pantheon_sample_timecycle(g_weather, g_day01);
        PantheonAtmosphere a_to = pantheon_sample_timecycle(w_target, g_day01);
        g_atmosphere_target = pantheon_lerp_atmosphere(a_from, a_to, bt);
        g_weather_blend_remaining--;
        if (g_weather_blend_remaining == 0) {
            g_weather = w_target;
        }
    } else {
        g_weather = w_target;
        g_atmosphere_target = pantheon_sample_timecycle(g_weather, g_day01);
    }

    /* Extra temporal smoothing: avoids stair-stepping when crossing timecycle slots. */
    if (!g_atmosphere_inited) {
        g_atmosphere = g_atmosphere_target;
        g_atmosphere_inited = 1;
    } else {
        float a = PANTHEON_ATMO_SMOOTH_ALPHA;
        if (a < 0.0f) {
            a = 0.0f;
        }
        if (a > 1.0f) {
            a = 1.0f;
        }
        g_atmosphere = pantheon_lerp_atmosphere(g_atmosphere, g_atmosphere_target, a);
    }
    update_skydome_colors();
}

/* Flattened triangle stream for VU1 (CPU expand at boot; see flight log Day 4). */
PantheonVertex flat_floor[FLOOR_TRI_COUNT * 3] __attribute__((aligned(16)));
#if PANTHEON_FLOOR_PATH1_DOUBLE_SIDED
/* Un-offset template: each mesh tri + same tri with swapped winding (proof branch technique). */
static PantheonVertex floor_path1_ds_template[PANTHEON_FLOOR_PATH1_VERTS] __attribute__((aligned(16)));
#endif
static PantheonVertex floor_tile_tris[PANTHEON_FLOOR_PATH1_VERTS] __attribute__((aligned(16)));
#if PANTHEON_TRIAGE_FORCE_FLAT_QUAD
static PantheonVertex floor_follow_quad[6] __attribute__((aligned(16)));
#endif

/* Walk bounds: X/Y/Z AABB of scaled floor mesh. */
static float floor_bound_x0, floor_bound_x1, floor_bound_y0, floor_bound_y1, floor_bound_z0, floor_bound_z1;
static float floor_tile_span_x = 0.0f, floor_tile_span_z = 0.0f;

static void init_floor_walk_bounds(void) {
    float minx = flat_floor[0].x;
    float maxx = minx;
    float miny = flat_floor[0].y;
    float maxy = miny;
    float minz = flat_floor[0].z;
    float maxz = minz;
    int n = FLOOR_TRI_COUNT * 3;
    for (int i = 1; i < n; i++) {
        float x = flat_floor[i].x;
        float y = flat_floor[i].y;
        float z = flat_floor[i].z;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
        if (z < minz) minz = z;
        if (z > maxz) maxz = z;
    }
    floor_bound_x0 = minx;
    floor_bound_x1 = maxx;
    floor_bound_y0 = miny;
    floor_bound_y1 = maxy;
    floor_bound_z0 = minz;
    floor_bound_z1 = maxz;
    floor_tile_span_x = floor_bound_x1 - floor_bound_x0;
    floor_tile_span_z = floor_bound_z1 - floor_bound_z0;
}

/* True when player is over the authored floor AABB (XZ), after init_floor_walk_bounds(). */
static int player_on_support_deck(void) {
    if (floor_tile_span_x <= 0.0f || floor_tile_span_z <= 0.0f) {
        return 1;
    }
    const float pad = 1.5f;
    return (player_x >= floor_bound_x0 - pad && player_x <= floor_bound_x1 + pad && player_z >= floor_bound_z0 - pad &&
            player_z <= floor_bound_z1 + pad);
}

static void clamp_player_to_floor(void) {
#if PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER
    /* Treadmill mode: infinite support surface under player. */
    player_y = 0.0f;
    return;
#endif
    if (player_on_support_deck()) {
        player_y = 0.0f;
    } else {
        player_y -= PANTHEON_PLAYER_ABYSS_FALL_STEP;
        if (player_y < PANTHEON_PLAYER_ABYSS_Y_MIN) {
            player_y = PANTHEON_PLAYER_ABYSS_Y_MIN;
        }
    }
    /* Keep world position in a huge envelope to avoid far-distance precision drift. */
    float limit_x = floor_tile_span_x * 100.0f;
    float limit_z = floor_tile_span_z * 100.0f;
    if (floor_tile_span_x > 0.0f) {
        if (player_x > limit_x) {
            player_x = limit_x;
        }
        if (player_x < -limit_x) {
            player_x = -limit_x;
        }
    }
    if (floor_tile_span_z > 0.0f) {
        if (player_z > limit_z) {
            player_z = limit_z;
        }
        if (player_z < -limit_z) {
            player_z = -limit_z;
        }
    }
}

static inline float snap_floor_axis(float value, float tile_span) {
    if (tile_span <= 0.0f) {
        return 0.0f;
    }
    return floorf(value / tile_span) * tile_span;
}

#if !USE_VU1_SKYDOME_MESH
/* Procedural sky hemisphere (GIF path) when no authored skydome tris. */
#define SKY_RADIUS 1600.0f
/* Latitude must not start at theta=0: sin(theta)=0 collapses the whole top ring to one
 * point (0,R,0), which degenerates every triangle touching it -> GS "vertex explosion" fan. */
#define SKY_THETA_MIN 0.07f
#define SKY_LAT 6
#define SKY_LON 16
#define SKY_VCOUNT ((SKY_LAT + 1) * (SKY_LON + 1))
#define SKY_TCOUNT (SKY_LAT * SKY_LON * 2)

static VECTOR sky_verts[SKY_VCOUNT];
static VECTOR sky_cols[SKY_VCOUNT];
static int sky_tri_a[SKY_TCOUNT];
static int sky_tri_b[SKY_TCOUNT];
static int sky_tri_c[SKY_TCOUNT];

static void init_sky_placeholder_dome(void) {
    int vi = 0;
    for (int i = 0; i <= SKY_LAT; i++) {
        float t = (float)i / (float)SKY_LAT;
        float theta = SKY_THETA_MIN + ((PI_F * 0.5f) - SKY_THETA_MIN) * t;
        float sin_t = sinf(theta);
        float cos_t = cosf(theta);
        for (int j = 0; j <= SKY_LON; j++) {
            float phi = (2.0f * PI_F) * (float)j / (float)SKY_LON;
            float c = cosf(phi);
            float s = sinf(phi);
            sky_verts[vi][0] = SKY_RADIUS * sin_t * c;
            sky_verts[vi][1] = SKY_RADIUS * cos_t;
            sky_verts[vi][2] = SKY_RADIUS * sin_t * s;
            sky_verts[vi][3] = 1.0f;

            float grid = (((i & 1) == 0) || ((j & 1) == 0)) ? 1.0f : 0.92f;
            float horizon = 1.0f - t;
            /* Upper hemisphere only: zenith deep blue -> horizon pale (XP Bliss vibe). */
            float r = (0.50f + 0.22f * horizon) * grid;
            float g = (0.62f + 0.28f * t) * grid;
            float bl = (0.92f - 0.08f * horizon) * grid;
            float puff = sinf((float)j * 0.55f + (float)i * 0.4f);
            if (puff > 0.65f && t > 0.15f) {
                float w = (puff - 0.65f) * 1.8f;
                r = fminf(1.0f, r + w * 0.35f);
                g = fminf(1.0f, g + w * 0.32f);
                bl = fminf(1.0f, bl + w * 0.12f);
            }
            {
                float alpha = (float)g_atmosphere.cloud_alpha / 255.0f;
                if (alpha > 0.0f && t > 0.15f) {
                    float puff = sinf((float)j * 0.55f + (float)i * 0.4f);
                    if (puff > 0.65f) {
                        float w = (puff - 0.65f) * 1.8f * alpha;
                        r = fminf(1.0f, r + (((float)g_atmosphere.cloud.r / 255.0f) - r) * w);
                        g = fminf(1.0f, g + (((float)g_atmosphere.cloud.g / 255.0f) - g) * w);
                        bl = fminf(1.0f, bl + (((float)g_atmosphere.cloud.b / 255.0f) - bl) * w);
                    }
                }
            }
            sky_cols[vi][0] = r;
            sky_cols[vi][1] = g;
            sky_cols[vi][2] = bl;
            sky_cols[vi][3] = 1.0f;
            vi++;
        }
    }

    int ti = 0;
    for (int i = 0; i < SKY_LAT; i++) {
        for (int j = 0; j < SKY_LON; j++) {
            int v0 = i * (SKY_LON + 1) + j;
            int v1 = (i + 1) * (SKY_LON + 1) + j;
            int v2 = i * (SKY_LON + 1) + (j + 1);
            int v3 = (i + 1) * (SKY_LON + 1) + (j + 1);
            sky_tri_a[ti] = v0;
            sky_tri_b[ti] = v1;
            sky_tri_c[ti] = v2;
            ti++;
            sky_tri_a[ti] = v1;
            sky_tri_b[ti] = v3;
            sky_tri_c[ti] = v2;
            ti++;
        }
    }
}

static qword_t *render_sky_placeholder_dome(qword_t *q, MATRIX world_view, MATRIX view_screen) {
    VECTOR obj_pos = {0.0f, 0.0f, 0.0f, 1.0f};
    VECTOR obj_rot = {0.0f, 0.0f, 0.0f, 1.0f};
    MATRIX local_world, local_screen;
    VECTOR tvert[SKY_VCOUNT];
    xyz_t xyz[SKY_VCOUNT];
    color_t col[SKY_VCOUNT];
    prim_t prim;
    color_t base;

    create_local_world(local_world, obj_pos, obj_rot);
    create_local_screen(local_screen, local_world, world_view, view_screen);
    calculate_vertices(tvert, SKY_VCOUNT, sky_verts, local_screen);
    draw_convert_xyz(xyz, 2048, 2048, 32, SKY_VCOUNT, (vertex_f_t *)tvert);
    draw_convert_rgbq(col, SKY_VCOUNT, (vertex_f_t *)tvert, (color_f_t *)sky_cols, 0x80);

    prim.type = PRIM_TRIANGLE;
    prim.shading = PRIM_SHADE_GOURAUD;
    prim.mapping = DRAW_DISABLE;
    prim.fogging = DRAW_DISABLE;
    prim.blending = DRAW_DISABLE;
    prim.antialiasing = DRAW_ENABLE;
    prim.mapping_type = PRIM_MAP_ST;
    prim.colorfix = PRIM_UNFIXED;

    base.r = 0x80;
    base.g = 0x80;
    base.b = 0x80;
    base.a = 0x80;
    base.q = 1.0f;
    q = draw_prim_start(q, 0, &prim, &base);
    for (int t = 0; t < SKY_TCOUNT; t++) {
        int ia = sky_tri_a[t];
        int ib = sky_tri_b[t];
        int ic = sky_tri_c[t];
        q->dw[0] = col[ia].rgbaq;
        q->dw[1] = xyz[ia].xyz;
        q++;
        q->dw[0] = col[ib].rgbaq;
        q->dw[1] = xyz[ib].xyz;
        q++;
        q->dw[0] = col[ic].rgbaq;
        q->dw[1] = xyz[ic].xyz;
        q++;
    }
    q = draw_prim_end(q, 2, DRAW_RGBAQ_REGLIST);
    return q;
}
#endif /* !USE_VU1_SKYDOME_MESH */

#define PHYSICAL(addr) ((u32)(addr) & 0x0FFFFFFF)

/* GS PACKED RGBAQ: EE packs RGBA bytes into .r as float; .g holds Q (1.0f) so GS doesn't zero-scale color. */
static inline float pantheon_rgbaq_from_u8(u8 r, u8 g, u8 b, u8 a) {
    PantheonColorPun pun;
    pun.i = ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
    return pun.f;
}

static void apply_sky_color(PantheonVertex *v) {
    /*
     * Spatial axis (dome): GTA SA-style latitude from vertex direction at dome center.
     * t = 0.5 * (y_hat + 1), y_hat = v.y / |v| — clamp to [0,1]: rim toward world -Y -> 0,
     * zenith +Y -> 1. Temporal axis is already folded into g_atmosphere (pantheon_sample_timecycle).
     */
    float len2 = v->x * v->x + v->y * v->y + v->z * v->z;
    float invl = (len2 > 1e-8f) ? (1.0f / sqrtf(len2)) : 0.0f;
    float t = 0.5f * ((v->y * invl) + 1.0f);
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    /* Spatial lerp: horizon -> zenith using active palette from temporal stage. */
    PantheonColor c = pantheon_lerp_color(g_atmosphere.sky_horizon, g_atmosphere.sky_top, t);
    unsigned char sky_a = pantheon_lerp_u8(200, 100, t);
#if PANTHEON_SKY_CLOUD_PUFF
    float horizon = 1.0f - t;
    float puff = sinf(v->x * 0.0045f + v->z * 0.0055f + v->y * 0.0025f);
    /* Optional puff: second linear RGB lerp toward cloud; weight capped to avoid magenta fringe. */
    if (puff > 0.48f && g_atmosphere.cloud_alpha > 0) {
        float w = (puff - 0.48f) * ((float)g_atmosphere.cloud_alpha / 255.0f);
        if (w > 0.55f)
            w = 0.55f;
        w *= horizon;
        c = pantheon_lerp_color(c, g_atmosphere.cloud, w);
    }
#endif

    v->r = pantheon_rgbaq_from_u8(c.r, c.g, c.b, sky_a);
    v->g = 1.0f;
    v->b = 0.0f;
    v->a = 0.0f;
}

static inline qword_t* add_dma_tag(qword_t *q, u16 qwc, u8 id, u32 addr, u32 vif0, u32 vif1) {
    q->sw[0] = (qwc & 0xFFFF) | ((id & 0x7) << 28);
    q->sw[1] = PHYSICAL(addr);
    q->sw[2] = vif0;
    q->sw[3] = vif1;
    return q + 1;
} 

typedef struct Path1MemoryImageBatch {
    const PantheonVertex *verts;
    int vert_count;
    u64 giftag0;
    u64 giftag1;
    float constants[8];
} Path1MemoryImageBatch;

static qword_t *path1_emit_shader_upload(qword_t *q, u32 shader_qwc, u32 shader_instr) {
    return add_dma_tag(q, shader_qwc, 3, (u32)&PantheonShader, 0, VIF_CODE(P_VIF_MPG, shader_instr, 0));
}

static qword_t *path1_emit_perspective_const(qword_t *q) {
    q = add_dma_tag(q, 1, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 1, PATH1_VU_CONST_PERSPECTIVE_ADDR));
    {
        float *f = (float *)q->sw;
        f[0] = 0.0f; f[1] = 0.0f; f[2] = 0.0f; f[3] = 1.0f;
    }
    return q + 1;
}

static void path1_build_memory_image(Path1MemoryImageBatch *batch, const PantheonVertex *verts, int vert_count) {
    batch->verts = verts;
    batch->vert_count = vert_count;
    /* PACKED contract: RGBAQ + ST + XYZ2 for current shader.vsm output path. */
    batch->giftag0 = GIF_SET_TAG(
        vert_count,
        1,
        1,
        GIF_SET_PRIM(GIF_PRIM_TRIANGLE, 1, 0, 0, 0, 0, 1, 0, 0),
        GIF_FLG_PACKED,
        (PANTHEON_PATH1_GIF_USE_ST ? 3 : 2));
#if PANTHEON_PATH1_GIF_USE_ST
    batch->giftag1 = (u64)GIF_REG_RGBAQ | ((u64)GIF_REG_ST << 4) | ((u64)GIF_REG_XYZ2 << 8);
#else
    batch->giftag1 = (u64)GIF_REG_RGBAQ | ((u64)GIF_REG_XYZ2 << 4);
#endif
    /* VF05.xy = GS raster center (12.4 space origin); must match draw_convert_xyz(..., 2048, 2048, ...). */
    batch->constants[0] = 2048.0f;
    batch->constants[1] = 2048.0f;
    batch->constants[2] = 8388608.0f;
    batch->constants[3] = 1.0f;
    batch->constants[4] = 16.0f;
    batch->constants[5] = 1048576.0f;
    batch->constants[6] = PANTHEON_VU_NEARZ; /* Near-Z threshold consumed by VU1. */
    batch->constants[7] = 2047.0f;
}

static qword_t *path1_emit_memory_image_batch(qword_t *q, const Path1MemoryImageBatch *batch, MATRIX mvp) {
    if (!q || !batch || !batch->verts || !mvp) {
        return q;
    }
    int batch_qwc = batch->vert_count * PATH1_VU_QW_PER_VERT;
    /* XITOP-driven loop in shader.vsm decrements by 3 qwords per vertex,
     * so ITOP must carry qword count, not vertex count. */
    int itops_val = batch_qwc;

    q = add_dma_tag(q, 1, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 1, PATH1_VU_GIFTAG_ADDR));
    q->dw[0] = batch->giftag0;
    q->dw[1] = batch->giftag1;
    q++;

    q = add_dma_tag(q, 2, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 2, PATH1_VU_CONST_SCREEN_ADDR));
    memcpy(q, batch->constants, sizeof(batch->constants));
    q += 2;

    q = add_dma_tag(q, 4, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 4, PATH1_VU_CONST_MVP_ADDR));
    memcpy(q, mvp, 64);
    q += 4;

    q = add_dma_tag(q, batch_qwc, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, batch_qwc, PATH1_VU_INPUT_BASE));
    memcpy(q, batch->verts, batch_qwc * 16);
    q += batch_qwc;

    q = add_dma_tag(q, 0, 1, 0, VIF_CODE(P_VIF_ITOP, 0, itops_val), VIF_CODE(P_VIF_MSCAL, 0, 0));
    q = add_dma_tag(q, 0, 1, 0, VIF_CODE(P_VIF_FLUSHE, 0, 0), 0);
    return q;
}

static packet_t *path1_vif_packet_begin(void) {
    packet_t *pkt = packets[g_vif_context];
    packet_reset(pkt);
    return pkt;
}

static void path1_vif_packet_submit(packet_t *pkt, qword_t *q_end, int frame_id, const char *tag) {
    (void)frame_id;
    (void)tag;
    int vif1_qw = (int)(q_end - pkt->data);
    g_last_vif1_qw = vif1_qw;
    dma_channel_send_chain(DMA_CHANNEL_VIF1, (void *)PHYSICAL(pkt->data), vif1_qw, 0, 0);
    dma_wait_fast();
    g_vif_context ^= 1;
}

// Unroll the indices into a flat triangle array at boot
void init_flat_floor() {
    int out_idx = 0;
    for (int i = 0; i < FLOOR_TRI_COUNT; i++) {
#if PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
        /* Path1 tiled floor: XY→XZ map flips handedness — (a,c,b) faces +Y for single-sided tris. */
        flat_floor[out_idx++] = floor_vertices[floor_indices[i].a];
        flat_floor[out_idx++] = floor_vertices[floor_indices[i].c];
        flat_floor[out_idx++] = floor_vertices[floor_indices[i].b];
#else
        flat_floor[out_idx++] = floor_vertices[floor_indices[i].a];
        flat_floor[out_idx++] = floor_vertices[floor_indices[i].b];
        flat_floor[out_idx++] = floor_vertices[floor_indices[i].c];
#endif
    }

    // Keep geometry in a sane range while validating VU1 transforms.
    // Safe type-punning using a union to satisfy strict-aliasing rules
    PantheonColorPun color_pun;
    for (int i = 0; i < FLOOR_TRI_COUNT * 3; i++) {
        flat_floor[i].x *= FLOOR_MESH_SCALE;
#if PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
        {
            float src_y = flat_floor[i].y;
            /* Source floor arrives in XY plane; map to engine XZ world plane. */
            flat_floor[i].y = 0.0f;
            flat_floor[i].z = src_y * FLOOR_MESH_SCALE;
        }
#else
        flat_floor[i].z *= FLOOR_MESH_SCALE;
        flat_floor[i].y *= FLOOR_MESH_SCALE;
#endif

        /* Sandbox grid: major lines on integer world units (after scale), darker cell fill. */
        {
            float gx = flat_floor[i].x;
            float gz = flat_floor[i].z;
            float ax = fabsf(gx);
            float az = fabsf(gz);
            float fx = ax - floorf(ax);
            float fz = az - floorf(az);
            int on_major = (fx < 0.04f || fx > 0.96f || fz < 0.04f || fz > 0.96f) ? 1 : 0;
            int cx = (int)floorf(ax + 0.5f);
            int cz = (int)floorf(az + 0.5f);
            int checker = ((cx + cz) & 1);
            u8 r, g, b, a;
            if (on_major) {
                r = 72;
                g = 200;
                b = 92;
                a = 255;
            } else if (checker) {
                r = 28;
                g = 92;
                b = 38;
                a = 230;
            } else {
                r = 22;
                g = 72;
                b = 30;
                a = 220;
            }
            color_pun.i = ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
        }

        // Build the exact 128-bit payload the GS RGBAQ register expects!
        flat_floor[i].r = color_pun.f; // Slot X: Pre-packed RGBA bytes
        flat_floor[i].g = 1.0f;        // Slot Y: The critical FLOAT Q-value!
        flat_floor[i].b = 0.0f;        // Slot Z: Padding
        flat_floor[i].a = 0.0f;        // Slot W: Padding
        flat_floor[i].s = 0.0f;
        flat_floor[i].t = 0.0f;
        flat_floor[i].tex_q = 1.0f;
        flat_floor[i].pad_uv = 0.0f;
    }

#if PANTHEON_FLOOR_PATH1_DOUBLE_SIDED
    for (int t = 0; t < FLOOR_TRI_COUNT; t++) {
        int b = t * 3;
        int o = t * 6;
        floor_path1_ds_template[o + 0] = flat_floor[b + 0];
        floor_path1_ds_template[o + 1] = flat_floor[b + 1];
        floor_path1_ds_template[o + 2] = flat_floor[b + 2];
        floor_path1_ds_template[o + 3] = flat_floor[b + 0];
        floor_path1_ds_template[o + 4] = flat_floor[b + 2];
        floor_path1_ds_template[o + 5] = flat_floor[b + 1];
    }
#endif
}

#if !PANTHEON_TRIAGE_FORCE_FLAT_QUAD
static void build_floor_tile(float tile_off_x, float tile_off_z) {
#if PANTHEON_FLOOR_PATH1_DOUBLE_SIDED
    const PantheonVertex *src = floor_path1_ds_template;
#else
    const PantheonVertex *src = flat_floor;
#endif
    for (int i = 0; i < PANTHEON_FLOOR_PATH1_VERTS; i++) {
        floor_tile_tris[i] = src[i];
        floor_tile_tris[i].x += tile_off_x;
        floor_tile_tris[i].z += tile_off_z;
    }
}
#endif

#if PANTHEON_TRIAGE_FORCE_FLAT_QUAD
static void build_floor_follow_quad(float center_x, float center_z) {
    float half_x = (floor_tile_span_x > 0.0f) ? (floor_tile_span_x * 0.5f) : 420.0f;
    float half_z = (floor_tile_span_z > 0.0f) ? (floor_tile_span_z * 0.5f) : 420.0f;
    /* Same vivid green as init_flat_floor() major grid lines (not the old muddy flat tint). */
    float floor_rgbaq = pantheon_rgbaq_from_u8(72, 200, 92, 255);

    float x0 = center_x - half_x;
    float x1 = center_x + half_x;
    float z0 = center_z - half_z;
    float z1 = center_z + half_z;
    float y = 0.0f;

    /* PantheonVertex order is x,y,z,w,r,g,b,a,s,t,tex_q,pad_uv — w must be 1.0f or RGBAQ lands in .w and color is garbage. */
    PantheonVertex v0 = {x0, y, z0, 1.0f, floor_rgbaq, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    PantheonVertex v1 = {x1, y, z0, 1.0f, floor_rgbaq, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
    PantheonVertex v2 = {x0, y, z1, 1.0f, floor_rgbaq, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f};
    PantheonVertex v3 = {x1, y, z1, 1.0f, floor_rgbaq, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f};

    floor_follow_quad[0] = v0;
    floor_follow_quad[1] = v2;
    floor_follow_quad[2] = v1;
    floor_follow_quad[3] = v1;
    floor_follow_quad[4] = v2;
    floor_follow_quad[5] = v3;
}
#endif

#if USE_VU1_SKYDOME_MESH
static void init_flat_skydome(void) {
    int out_idx = 0;
    for (int i = 0; i < SKYDOME_TRI_COUNT; i++) {
        /* Inward winding: viewer sits under the dome; Softimage outward normals need
         * (a,c,b) so GS front-faces point inward. Re-export an upper hemisphere only
         * from Softimage (y >= 0 cap) to save tris — same winding rule applies. */
        flat_skydome[out_idx++] = skydome_vertices[skydome_indices[i].a];
        flat_skydome[out_idx++] = skydome_vertices[skydome_indices[i].c];
        flat_skydome[out_idx++] = skydome_vertices[skydome_indices[i].b];
    }

    for (int i = 0; i < SKYDOME_TRI_COUNT * 3; i++) {
        PantheonVertex *v = &flat_skydome[i];
        v->x *= SKYDOME_MESH_SCALE;
        v->y *= SKYDOME_MESH_SCALE;
        v->z *= SKYDOME_MESH_SCALE;
        apply_sky_color(v);
        v->s = skydome_uv[i].s;
        v->t = skydome_uv[i].t;
        v->tex_q = skydome_uv[i].q;
        v->pad_uv = skydome_uv[i].pad;
    }
}
#endif

static qword_t *render_path1_tris(qword_t *q, MATRIX mvp, const PantheonVertex *mesh_tris, int total_verts, int max_verts) {
    u32 u32_count = (u32)(&PantheonShaderEnd - &PantheonShader);
    u32 shader_qwc = u32_count / 4;
    u32 shader_instr = u32_count / 2;

    q = path1_emit_shader_upload(q, shader_qwc, shader_instr);
    q = path1_emit_perspective_const(q);

    if (total_verts > max_verts) total_verts = max_verts;
    if (total_verts <= 0) return q;
    int verts_per_batch = PATH1_VU_MAX_BATCH_VERTS;
    g_last_nearz_hits = 0;

    for (int i = 0; i < total_verts; i += verts_per_batch) {
        Path1MemoryImageBatch batch;
        int batch_nearz_hits = 0;
        int verts_this_batch = total_verts - i;
        if (verts_this_batch > verts_per_batch) verts_this_batch = verts_per_batch;
        verts_this_batch -= (verts_this_batch % 3);
        if (verts_this_batch <= 0) {
            continue;
        }

        for (int sv = 0; sv < verts_this_batch; sv++) {
            const PantheonVertex *v = &mesh_tris[i + sv];
            float cw = (mvp[3] * v->x) + (mvp[7] * v->y) + (mvp[11] * v->z) + (mvp[15] * v->w);
            if (cw <= PANTHEON_VU_NEARZ) {
                batch_nearz_hits++;
            }
        }

        path1_build_memory_image(&batch, &mesh_tris[i], verts_this_batch);
        q = path1_emit_memory_image_batch(q, &batch, mvp);
        g_last_batch_verts = verts_this_batch;
        g_last_nearz_hits += batch_nearz_hits;
    }

    return q;
}

static void update_skydome_colors(void) {
#if USE_VU1_SKYDOME_MESH
    for (int i = 0; i < SKYDOME_TRI_COUNT * 3; i++) {
        apply_sky_color(&flat_skydome[i]);
    }
#endif
}

qword_t *render_floor_path1_count(qword_t *q, MATRIX mvp, int total_verts) {
#if PANTHEON_FLOOR_PATH1_DOUBLE_SIDED
    return render_path1_tris(q, mvp, floor_tile_tris, total_verts, PANTHEON_FLOOR_PATH1_VERTS);
#else
    return render_path1_tris(q, mvp, flat_floor, total_verts, FLOOR_TRI_COUNT * 3);
#endif
}

static int pantheon_boot_reveal_active(int frame_id) {
#if PANTHEON_BOOT_REVEAL_ENABLE
    return frame_id <= PANTHEON_BOOT_REVEAL_TOTAL_FRAMES;
#else
    (void)frame_id;
    return 0;
#endif
}

static u8 pantheon_boot_reveal_luma(int frame_id) {
#if PANTHEON_BOOT_REVEAL_ENABLE
    int half = PANTHEON_BOOT_REVEAL_HALF_FRAMES;
    int hold = PANTHEON_BOOT_REVEAL_HOLD_FRAMES;
    int f = frame_id;
    if (f <= 0) {
        return 0u;
    }
    if (f <= half) {
        int lum = f * PANTHEON_BOOT_REVEAL_STEP;
        if (lum > 255) {
            lum = 255;
        }
        return (u8)lum;
    }
    if (f <= (half + hold)) {
        return 255u;
    }
    if (f <= (half + hold + half)) {
        int down = f - (half + hold);
        int lum = 255 - (down * PANTHEON_BOOT_REVEAL_STEP);
        if (lum < 0) {
            lum = 0;
        }
        return (u8)lum;
    }
    return 0u;
#else
    (void)frame_id;
    return 0u;
#endif
}

static void pantheon_boot_glyph_rows(char c, u8 rows[7]) {
    rows[0] = 0x00; rows[1] = 0x00; rows[2] = 0x00; rows[3] = 0x00;
    rows[4] = 0x00; rows[5] = 0x00; rows[6] = 0x00;
    switch (c) {
        case '.': rows[5]=0x04; break;
        case '4': rows[0]=0x11; rows[1]=0x11; rows[2]=0x1F; rows[3]=0x01; rows[4]=0x01; break;
        case '9': rows[0]=0x0E; rows[1]=0x11; rows[2]=0x0F; rows[3]=0x01; rows[4]=0x0E; break;
        case 'B': rows[0]=0x1E; rows[1]=0x11; rows[2]=0x1E; rows[3]=0x11; rows[4]=0x11; rows[5]=0x1E; break;
        case 'C': rows[0]=0x0E; rows[1]=0x11; rows[2]=0x10; rows[3]=0x10; rows[4]=0x11; rows[5]=0x0E; break;
        case 'D': rows[0]=0x1E; rows[1]=0x11; rows[2]=0x11; rows[3]=0x11; rows[4]=0x11; rows[5]=0x1E; break;
        case 'I': rows[0]=0x1F; rows[1]=0x04; rows[2]=0x04; rows[3]=0x04; rows[4]=0x04; rows[5]=0x1F; break;
        case 'L': rows[0]=0x10; rows[1]=0x10; rows[2]=0x10; rows[3]=0x10; rows[4]=0x10; rows[5]=0x1F; break;
        case 'M': rows[0]=0x11; rows[1]=0x1B; rows[2]=0x15; rows[3]=0x15; rows[4]=0x11; rows[5]=0x11; break;
        case 'O': rows[0]=0x0E; rows[1]=0x11; rows[2]=0x11; rows[3]=0x11; rows[4]=0x11; rows[5]=0x0E; break;
        case 'S': rows[0]=0x0F; rows[1]=0x10; rows[2]=0x0E; rows[3]=0x01; rows[4]=0x01; rows[5]=0x1E; break;
        case 'T': rows[0]=0x1F; rows[1]=0x04; rows[2]=0x04; rows[3]=0x04; rows[4]=0x04; rows[5]=0x04; break;
        case 'U': rows[0]=0x11; rows[1]=0x11; rows[2]=0x11; rows[3]=0x11; rows[4]=0x11; rows[5]=0x0E; break;
        case 'W': rows[0]=0x11; rows[1]=0x11; rows[2]=0x11; rows[3]=0x15; rows[4]=0x15; rows[5]=0x1B; break;
        case 'Y': rows[0]=0x11; rows[1]=0x11; rows[2]=0x0A; rows[3]=0x04; rows[4]=0x04; rows[5]=0x04; break;
        default: break;
    }
}

static int pantheon_boot_glyph_advance(char c, int advance, int space_advance, int dot_advance) {
    if (c == ' ') {
        return space_advance;
    }
    if (c == '.') {
        return dot_advance;
    }
    return advance;
}

static qword_t *render_boot_title_overlay(qword_t *q, int frame_id) {
    static const char *title = "WWW.94BILLY.COM";
    int scale = PANTHEON_BOOT_TITLE_SCALE;
    const int advance = 6;
    const int space_advance = 3;
    const int dot_advance = 4;
    const int glyph_h = 7;
    const int z = 1;
    rect_t rect;
    int min_u_y = 9999;
    int max_u_y = -9999;
    int ink_found = 0;

    if (scale < 2) {
        scale = 2;
    }
    if (scale > 4) {
        scale = 4;
    }
    int cursor_units = 0;

    /* Measure ink vertical bounds; horizontal width uses layout_end_units (same advances as draw). */
    for (int gi = 0; title[gi] != '\0'; gi++) {
        char c = title[gi];
        if (c == ' ') {
            cursor_units += space_advance;
            continue;
        }
        if (c == '.') {
            u8 rows_dot[7];
            pantheon_boot_glyph_rows(c, rows_dot);
            for (int ry = 0; ry < glyph_h; ry++) {
                u8 bits = rows_dot[ry];
                for (int rx = 0; rx < 5; rx++) {
                    if ((bits & (1u << (4 - rx))) == 0) {
                        continue;
                    }
                    int uy0 = ry;
                    int uy1 = uy0 + 1;
                    if (uy0 < min_u_y) min_u_y = uy0;
                    if (uy1 > max_u_y) max_u_y = uy1;
                    ink_found = 1;
                }
            }
            cursor_units += dot_advance;
            continue;
        }
        u8 rows[7];
        pantheon_boot_glyph_rows(c, rows);
        for (int ry = 0; ry < glyph_h; ry++) {
            u8 bits = rows[ry];
            for (int rx = 0; rx < 5; rx++) {
                if ((bits & (1u << (4 - rx))) == 0) {
                    continue;
                }
                int uy0 = ry;
                int uy1 = uy0 + 1;
                if (uy0 < min_u_y) min_u_y = uy0;
                if (uy1 > max_u_y) max_u_y = uy1;
                ink_found = 1;
            }
        }
        cursor_units += advance;
    }

    if (!ink_found) {
        return q;
    }

    /* Full line width in cursor units (must match render loop: spaces + per-glyph advance).
     * Centering on ink bbox alone shifts the string when spaces are present. */
    int layout_end_units = 0;
    for (int gi = 0; title[gi] != '\0'; gi++) {
        layout_end_units += pantheon_boot_glyph_advance(title[gi], advance, space_advance, dot_advance);
    }

    int height_units = max_u_y - min_u_y;
    int width_units_layout = layout_end_units;
    while (scale > 2 && ((width_units_layout * scale) > 620 || (height_units * scale) > 128)) {
        scale--;
    }

    int height_px = height_units * scale;
    /* Center full title line in 640×448 framebuffer (draw xyoffset is applied separately). */
    int start_x = (640 - layout_end_units * scale) / 2;
    int start_y = (448 - height_px) / 2;

    /* Contrast with monochrome boot ramp: dark bg -> light glyphs, peak white -> dark glyphs. */
    {
        u8 lum = pantheon_boot_reveal_luma(frame_id);
        u8 ink = (lum < 200u) ? 0xFFu : 0x00u;
        rect.color.r = ink;
        rect.color.g = ink;
        rect.color.b = ink;
        rect.color.a = 0xFF;
        rect.color.q = 1.0f;
    }

    cursor_units = 0;
    for (int gi = 0; title[gi] != '\0'; gi++) {
        char c = title[gi];
        if (c == ' ') {
            cursor_units += space_advance;
            continue;
        }
        u8 rows[7];
        pantheon_boot_glyph_rows(c, rows);
        for (int ry = 0; ry < 7; ry++) {
            u8 bits = rows[ry];
            for (int rx = 0; rx < 5; rx++) {
                if ((bits & (1u << (4 - rx))) == 0) {
                    continue;
                }
                int px = start_x + ((cursor_units + rx) * scale);
                int py = start_y + ((ry - min_u_y) * scale);
                /* Match GS framebuffer origin used by draw_clear / draw_primitive_xyoffset (ps2sdk draw2d START/END offsets). */
                rect.v0.x = PANTHEON_DRAW_GS_FB_X0 + (float)px - PANTHEON_LIBDRAW_START_OFFSET;
                rect.v0.y = PANTHEON_DRAW_GS_FB_Y0 + (float)py - PANTHEON_LIBDRAW_START_OFFSET;
                rect.v0.z = z;
                rect.v1.x = PANTHEON_DRAW_GS_FB_X0 + (float)(px + scale) - PANTHEON_LIBDRAW_END_OFFSET;
                rect.v1.y = PANTHEON_DRAW_GS_FB_Y0 + (float)(py + scale) - PANTHEON_LIBDRAW_END_OFFSET;
                rect.v1.z = z;
                q = draw_rect_filled(q, 0, &rect);
            }
        }
        cursor_units += pantheon_boot_glyph_advance(c, advance, space_advance, dot_advance);
    }

    return q;
}

qword_t *render_clear_and_setup(qword_t *q, int ctx, framebuffer_t *frame, zbuffer_t *z, int frame_id) {
    u8 clear_r = g_atmosphere.sky_horizon.r;
    u8 clear_g = g_atmosphere.sky_horizon.g;
    u8 clear_b = g_atmosphere.sky_horizon.b;
    if (pantheon_boot_reveal_active(frame_id)) {
        u8 luma = pantheon_boot_reveal_luma(frame_id);
        clear_r = luma;
        clear_g = luma;
        clear_b = luma;
    }
    q = draw_setup_environment(q, 0, &frame[ctx], z);
    q = draw_primitive_xyoffset(q, 0, 2048 - 320, 2048 - 224);
    /* Optional boot reveal phase: monochrome luma ramp (black->white->black), then normal sky clear color. */
    q = draw_clear(
        q,
        0,
        2048.0f - 320.0f,
        2048.0f - 224.0f,
        640.0f,
        448.0f,
        clear_r,
        clear_g,
        clear_b);
    return q;
}

static qword_t *render_cpu_probe(qword_t *q, MATRIX world_view, MATRIX view_screen) {
    static VECTOR probe_verts[3] = {
        {-60.0f, 0.0f, -60.0f, 1.0f},
        { 60.0f, 0.0f, -60.0f, 1.0f},
        {  0.0f, 0.0f,  60.0f, 1.0f}
    };
    static VECTOR probe_cols[3] = {
        {1.0f, 0.2f, 0.2f, 1.0f},
        {0.2f, 1.0f, 0.2f, 1.0f},
        {0.2f, 0.2f, 1.0f, 1.0f}
    };
    static int probe_idx[3] = {0, 1, 2};
    VECTOR obj_pos = {0.0f, 0.0f, 0.0f, 1.0f};
    VECTOR obj_rot = {0.0f, 0.0f, 0.0f, 1.0f};
    MATRIX local_world, local_screen;
    VECTOR tvert[3];
    xyz_t xyz[3];
    color_t col[3];
    prim_t prim;
    color_t base;

    create_local_world(local_world, obj_pos, obj_rot);
    create_local_screen(local_screen, local_world, world_view, view_screen);
    calculate_vertices(tvert, 3, probe_verts, local_screen);
    draw_convert_xyz(xyz, 2048, 2048, 32, 3, (vertex_f_t *)tvert);
    draw_convert_rgbq(col, 3, (vertex_f_t *)tvert, (color_f_t *)probe_cols, 0x80);

    prim.type = PRIM_TRIANGLE;
    prim.shading = PRIM_SHADE_GOURAUD;
    prim.mapping = DRAW_DISABLE;
    prim.fogging = DRAW_DISABLE;
    prim.blending = DRAW_DISABLE;
    prim.antialiasing = DRAW_ENABLE;
    prim.mapping_type = PRIM_MAP_ST;
    prim.colorfix = PRIM_UNFIXED;

    base.r = 0x80; base.g = 0x80; base.b = 0x80; base.a = 0x80; base.q = 1.0f;
    q = draw_prim_start(q, 0, &prim, &base);
    for (int i = 0; i < 3; i++) {
        q->dw[0] = col[probe_idx[i]].rgbaq;
        q->dw[1] = xyz[probe_idx[i]].xyz;
        q++;
    }
    q = draw_prim_end(q, 2, DRAW_RGBAQ_REGLIST);
    return q;
}

static qword_t *render_cpu_exported_floor(qword_t *q, MATRIX world_view, MATRIX view_screen) {
    static VECTOR floor_pos = {0.0f, 0.0f, 0.0f, 1.0f};
    static VECTOR floor_rot = {0.0f, 0.0f, 0.0f, 1.0f};
    static VECTOR floor_positions[FLOOR_VERT_COUNT];
    static VECTOR floor_cols[FLOOR_VERT_COUNT];
    static int floor_arrays_init = 0;

    MATRIX local_world, local_screen;
    VECTOR tvert[FLOOR_VERT_COUNT];
    xyz_t xyz[FLOOR_VERT_COUNT];
    color_t col[FLOOR_VERT_COUNT];
    prim_t prim;
    color_t base;

    if (!floor_arrays_init) {
        for (int i = 0; i < FLOOR_VERT_COUNT; i++) {
#if PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
            /* Match Path1 floor swizzle so hybrid profile doesn't diverge from VU1 floor. */
            floor_positions[i][0] = floor_vertices[i].x * FLOOR_MESH_SCALE;
            floor_positions[i][1] = 0.0f;
            floor_positions[i][2] = floor_vertices[i].y * FLOOR_MESH_SCALE;
#else
            floor_positions[i][0] = floor_vertices[i].x * FLOOR_MESH_SCALE;
            floor_positions[i][1] = floor_vertices[i].y * FLOOR_MESH_SCALE;
            floor_positions[i][2] = floor_vertices[i].z * FLOOR_MESH_SCALE;
#endif
            floor_positions[i][3] = floor_vertices[i].w;
            float r = floor_vertices[i].r;
            float g = floor_vertices[i].g;
            float b = floor_vertices[i].b;
            float a = floor_vertices[i].a;
            if (r > 1.0f) r /= 255.0f;
            if (g > 1.0f) g /= 255.0f;
            if (b > 1.0f) b /= 255.0f;
            if (a > 1.0f) a /= 255.0f;
            /* Bias GIF overlay toward sunlit grass (Bliss-style); keeps grid contrast. */
            floor_cols[i][0] = r * 0.28f + 0.07f;
            floor_cols[i][1] = fminf(1.0f, g * 0.42f + 0.50f);
            floor_cols[i][2] = b * 0.32f + 0.11f;
            floor_cols[i][3] = a;
        }
        floor_arrays_init = 1;
    }

    create_local_world(local_world, floor_pos, floor_rot);
    create_local_screen(local_screen, local_world, world_view, view_screen);
    calculate_vertices(tvert, FLOOR_VERT_COUNT, floor_positions, local_screen);
    draw_convert_xyz(xyz, 2048, 2048, 32, FLOOR_VERT_COUNT, (vertex_f_t *)tvert);
    draw_convert_rgbq(col, FLOOR_VERT_COUNT, (vertex_f_t *)tvert, (color_f_t *)floor_cols, 0x80);

    prim.type = PRIM_TRIANGLE;
    prim.shading = PRIM_SHADE_GOURAUD;
    prim.mapping = DRAW_DISABLE;
    prim.fogging = DRAW_DISABLE;
    prim.blending = DRAW_DISABLE;
    prim.antialiasing = DRAW_ENABLE;
    prim.mapping_type = PRIM_MAP_ST;
    prim.colorfix = PRIM_UNFIXED;

    base.r = 0x80; base.g = 0x80; base.b = 0x80; base.a = 0x80; base.q = 1.0f;
    q = draw_prim_start(q, 0, &prim, &base);
    for (int i = 0; i < FLOOR_TRI_COUNT; i++) {
        int ia = floor_indices[i].a;
        int ib = floor_indices[i].b;
        int ic = floor_indices[i].c;
#if PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
        /* Keep CPU overlay winding aligned with Path1 floor winding. */
        q->dw[0] = col[ia].rgbaq; q->dw[1] = xyz[ia].xyz; q++;
        q->dw[0] = col[ic].rgbaq; q->dw[1] = xyz[ic].xyz; q++;
        q->dw[0] = col[ib].rgbaq; q->dw[1] = xyz[ib].xyz; q++;
#else
        q->dw[0] = col[ia].rgbaq; q->dw[1] = xyz[ia].xyz; q++;
        q->dw[0] = col[ib].rgbaq; q->dw[1] = xyz[ib].xyz; q++;
        q->dw[0] = col[ic].rgbaq; q->dw[1] = xyz[ic].xyz; q++;
#endif
    }
    q = draw_prim_end(q, 2, DRAW_RGBAQ_REGLIST);
    return q;
}

// ==================== MAIN ====================

int main(int argc, char *argv[]) {
    typedef char pantheon_vertex_size_must_be_48[(sizeof(PantheonVertex) == 48) ? 1 : -1];
    typedef char pantheon_tri_size_must_be_16[(sizeof(PantheonTriangle) == 16) ? 1 : -1];
    typedef char pantheon_vertex_align_must_be_16[((__alignof__(PantheonVertex) == 16)) ? 1 : -1];
    typedef char pantheon_tri_align_must_be_16[((__alignof__(PantheonTriangle) == 16)) ? 1 : -1];
    (void)sizeof(pantheon_vertex_size_must_be_48);
    (void)sizeof(pantheon_tri_size_must_be_16);
    (void)sizeof(pantheon_vertex_align_must_be_16);
    (void)sizeof(pantheon_tri_align_must_be_16);
    {
        typedef char pantheon_vu_batch_qw_guard[(PATH1_VU_BATCH_QW <= 256) ? 1 : -1];
        (void)sizeof(pantheon_vu_batch_qw_guard);
    }

    SifInitRpc(0);

    int sio2_res = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    int padman_res = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (sio2_res < 0 || padman_res < 0) {
        return 1;
    }

    padInit(0);
    int pad_open_res = padPortOpen(0, 0, padBuf);
    pad_port_open_ok = (pad_open_res != 0);

    framebuffer_t frame[2];
    zbuffer_t z;
    unsigned int ext_fb_words = pantheon_vram_surface_extent_words(640, 448, GS_PSM_32, GRAPH_ALIGN_PAGE);
    unsigned int ext_z_words = pantheon_vram_surface_extent_words(640, 448, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

    frame[0].width = 640; frame[0].height = 448; frame[0].psm = GS_PSM_32;
    frame[1].width = 640; frame[1].height = 448; frame[1].psm = GS_PSM_32;

    pantheon_vram_linear_reset();
    frame[0].address = pantheon_vram_linear_alloc_words(ext_fb_words, PANTHEON_VRAM_PAGE_WORDS);
    frame[1].address = pantheon_vram_linear_alloc_words(ext_fb_words, PANTHEON_VRAM_PAGE_WORDS);

    z.enable = DRAW_ENABLE; z.mask = 0; z.method = ZTEST_METHOD_ALLPASS; z.zsm = GS_PSMZ_32;
    z.address = pantheon_vram_linear_alloc_words(ext_z_words, PANTHEON_VRAM_PAGE_WORDS);

    if (frame[0].address == 0u || frame[1].address == 0u || z.address == 0u) {
        printf("pantheon: pantheon_vram_linear_alloc_words failed for framebuffer or Z\n");
    }

    graph_initialize(frame[0].address, frame[0].width, frame[0].height, frame[0].psm, 0, 0);
    graph_set_framebuffer(0, frame[0].address, frame[0].width, frame[0].psm, 0, 0);
    graph_enable_output();

    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
    /* Intermittent Mode Transfer: Path 3 texture uploads yield to Path 1 every 8 QW (verify vs SCE). */
    GIF_REG_MODE |= (1u << 2);

    packets[0] = packet_init(4000, PACKET_NORMAL);
    packets[1] = packet_init(4000, PACKET_NORMAL);

    {
        texbuffer_t pantheon_host_tex;
        int pantheon_tex_ok = 0;
        unsigned ext_fb;
        unsigned ext_z;
        unsigned ext_tex = 0u;
        unsigned tbp = 0u;

        pantheon_host_texture_boot(&pantheon_host_tex, &pantheon_tex_ok);
        ext_fb = ext_fb_words;
        ext_z = ext_z_words;
        if (pantheon_tex_ok) {
            ext_tex = pantheon_vram_surface_extent_words(64, 64, GS_PSM_32, GRAPH_ALIGN_BLOCK);
            tbp = pantheon_host_tex.address;
        }
        pantheon_vram_log_layout(
            (unsigned int)frame[0].address,
            ext_fb,
            (unsigned int)frame[1].address,
            ext_fb,
            (unsigned int)z.address,
            ext_z,
            tbp,
            ext_tex,
            pantheon_tex_ok);
#if !PANTHEON_TEXTURE_PHASE
        (void)pantheon_host_tex;
#endif
    }

    init_flat_floor();
    init_floor_walk_bounds();
    MATRIX world_view, view_screen, mvp;
#if PANTHEON_VISUAL_PRESET != 1
    MATRIX sky_local_world;
#endif
    MATRIX sky_local_screen;
    /* Sample atmosphere before skydome vertex colors so apply_sky_color sees valid g_atmosphere. */
    g_atmosphere = pantheon_sample_timecycle(g_weather, g_day01);
#if USE_VU1_SKYDOME_MESH
    init_flat_skydome();
#endif

    update_atmosphere(0);
    create_view_screen(
        view_screen,
        graph_aspect_ratio(),
        -PANTHEON_VIEW_FRUSTUM_LR,
        PANTHEON_VIEW_FRUSTUM_LR,
        -PANTHEON_VIEW_FRUSTUM_BT,
        PANTHEON_VIEW_FRUSTUM_BT,
        PANTHEON_VIEW_NEAR,
        g_atmosphere.far_clip);
    printf("pantheon visual_preset=%d strict_profile=%d overlay=%d tile_radius=%d tile_budget=%d\n",
           PANTHEON_VISUAL_PRESET,
           PANTHEON_RENDER_PROFILE, PATH1_AB_CPU_OVERLAY, PANTHEON_TILE_RADIUS, PANTHEON_TILE_SUBMIT_BUDGET);
    printf("pantheon triage static_cam=%d fpv=%d eye_h=%.1f sky=%d view_near=%.2f vu_nearz=%.2f envelope=%d\n",
           PANTHEON_TRIAGE_STATIC_CAMERA,
           PANTHEON_TRIAGE_FIRST_PERSON,
           PANTHEON_FPV_EYE_HEIGHT,
           PANTHEON_TRIAGE_ENABLE_SKYDOME,
           PANTHEON_VIEW_NEAR,
           PANTHEON_VU_NEARZ,
           PANTHEON_CAMERA_ENVELOPE_HARDEN);

    while (1) {
        static int dbg_frame = 0;
        dbg_frame++;
        update_atmosphere(dbg_frame);
        packet_reset(packets[context]);
        qword_t *q = packets[context]->data;
        int boot_reveal = pantheon_boot_reveal_active(dbg_frame);
        const PantheonRenderJob render_jobs[] = {
            {PANTHEON_RENDER_JOB_CLEAR, 1},
            {PANTHEON_RENDER_JOB_CPU_DEBUG, PATH1_AB_CPU_OVERLAY && !boot_reveal},
            {PANTHEON_RENDER_JOB_PATH1_SKY,
             (RENDER_STAGE >= 2) && !boot_reveal && PANTHEON_TRIAGE_ENABLE_SKYDOME && !PANTHEON_TRIAGE_DISABLE_SKY_PASS},
            {PANTHEON_RENDER_JOB_PATH1_FLOOR, (RENDER_STAGE >= 2) && !boot_reveal}
        };

#if PANTHEON_MIN_TELEMETRY
        if ((dbg_frame % 300) == 0) {
            printf("pantheon frame=%d overlay=%d cam=(%.2f,%.2f,%.2f)\n",
                   dbg_frame,
                   PATH1_AB_CPU_OVERLAY,
                   camera_position[0], camera_position[1], camera_position[2]);
        }
#endif

        try_finish_pad_init();
        /* Ground invariant: keep Y anchored on the deck before input integration. */
        if (player_on_support_deck()) {
            player_y = 0.0f;
        }
        read_pad_analog();
        /* Ground invariant: while supported by the floor deck, lock vertical position. */
        if (player_on_support_deck()) {
            player_y = 0.0f;
        }
#if !PANTHEON_TRIAGE_STATIC_CAMERA
        update_camera_orbit();
#else
        /* Fixed eye; use same pitch/yaw convention as update_camera_orbit (never +cam_pitch here). */
        camera_position[0] = 0.0f;
        camera_position[1] = 63.0f;
        camera_position[2] = 101.0f;
        camera_position[3] = 1.0f;
        camera_rotation[0] = PANTHEON_TRIAGE_PITCH_SIGN_FLIP ? cam_pitch : -cam_pitch;
        camera_rotation[1] = cam_yaw;
        camera_rotation[2] = 0.0f;
        camera_rotation[3] = 1.0f;
#endif

        create_view_screen(
        view_screen,
        graph_aspect_ratio(),
        -PANTHEON_VIEW_FRUSTUM_LR,
        PANTHEON_VIEW_FRUSTUM_LR,
        -PANTHEON_VIEW_FRUSTUM_BT,
        PANTHEON_VIEW_FRUSTUM_BT,
        PANTHEON_VIEW_NEAR,
        g_atmosphere.far_clip);
        {
            VECTOR world_view_rotation;
            memcpy(world_view_rotation, camera_rotation, sizeof(VECTOR));
#if PANTHEON_TRIAGE_WORLDVIEW_YAW_GUARD
            world_view_rotation[1] = 0.0f;
#endif
            create_world_view(world_view, camera_position, world_view_rotation);
        }
        matrix_multiply(mvp, world_view, view_screen);
#if PANTHEON_VISUAL_PRESET == 1
        /* Apr 30 stable: skydome used world mvp (dome at origin, no player-relative lift). */
        memcpy(sky_local_screen, mvp, sizeof(MATRIX));
#else
        {
            VECTOR sky_pos = {
                player_x,
                player_y + PANTHEON_SKYDOME_PLAYER_Y_LIFT,
                player_z,
                1.0f
            };
            VECTOR sky_rot = {0.0f, 0.0f, 0.0f, 1.0f};
            create_local_world(sky_local_world, sky_pos, sky_rot);
            create_local_screen(sky_local_screen, sky_local_world, world_view, view_screen);
        }
#endif

        if (render_jobs[0].enabled) {
            q = render_clear_and_setup(q, context, frame, &z, dbg_frame);
            if (boot_reveal) {
                q = render_boot_title_overlay(q, dbg_frame);
            }
        }

        if (render_jobs[1].enabled) {
            q = render_cpu_probe(q, world_view, view_screen);
            q = render_cpu_exported_floor(q, world_view, view_screen);
        }

        FlushCache(0);
        {
            int gif_qw = (int)(q - packets[context]->data);
            dma_channel_send_normal(DMA_CHANNEL_GIF, (void *)PHYSICAL(packets[context]->data), gif_qw, 0, 0);
        }
        dma_wait_fast();

        if (render_jobs[2].enabled || render_jobs[3].enabled) {
        #if USE_VU1_SKYDOME_MESH
            /* Path1 skydome first, then floor. */
            if (render_jobs[2].enabled) {
                packet_t *vif_pkt = path1_vif_packet_begin();
                q = vif_pkt->data;
                g_path1_mesh_stage = "skydome";
                q = render_path1_tris(q, sky_local_screen, flat_skydome, SKYDOME_TRI_COUNT * 3, SKYDOME_TRI_COUNT * 3);
                q = add_dma_tag(q, 0, 7, 0, 0, 0);
                FlushCache(0);
                path1_vif_packet_submit(vif_pkt, q, dbg_frame, "skydome");
            }
        #endif
            if (render_jobs[3].enabled) {
                int path1_vert_count = PANTHEON_FLOOR_PATH1_VERTS;
                int tile_submits = 0;
#if PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER
                {
                    packet_t *vif_pkt = path1_vif_packet_begin();
                    q = vif_pkt->data;
                    /* Treadmill behavior: snap follow patch to fixed gameplay tile boundaries. */
                    g_floor_center_x = snap_floor_axis(player_x, PANTHEON_FLOOR_FOLLOW_TILE_SIZE);
                    g_floor_center_z = snap_floor_axis(player_z, PANTHEON_FLOOR_FOLLOW_TILE_SIZE);
#if PANTHEON_TRIAGE_FORCE_FLAT_QUAD
                    build_floor_follow_quad(g_floor_center_x, g_floor_center_z);
                    path1_vert_count = 6;
#else
                    build_floor_tile(g_floor_center_x, g_floor_center_z);
#endif
                    g_path1_mesh_stage = "floor_player";
#if PANTHEON_TRIAGE_FORCE_FLAT_QUAD
                    q = render_path1_tris(q, mvp, floor_follow_quad, path1_vert_count, path1_vert_count);
#else
                    q = render_path1_tris(q, mvp, floor_tile_tris, path1_vert_count, path1_vert_count);
#endif
                    q = add_dma_tag(q, 0, 7, 0, 0, 0);
                    FlushCache(0);
                    path1_vif_packet_submit(vif_pkt, q, dbg_frame, "floor_player");
                    tile_submits = 1;
                }
#else
                int tile_radius = PANTHEON_TILE_RADIUS;
                int tile_budget = PANTHEON_TILE_SUBMIT_BUDGET;
                if (floor_tile_span_x > 0.0f && floor_tile_span_z > 0.0f) {
                    int player_tile_x = (int)floorf(player_x / floor_tile_span_x);
                    int player_tile_z = (int)floorf(player_z / floor_tile_span_z);
                    for (int tz = -tile_radius; tz <= tile_radius; tz++) {
                        for (int tx = -tile_radius; tx <= tile_radius; tx++) {
                            if (tile_submits >= tile_budget) {
                                break;
                            }
                            packet_t *vif_pkt = path1_vif_packet_begin();
                            q = vif_pkt->data;
                            float tile_off_x = (float)(player_tile_x + tx) * floor_tile_span_x;
                            float tile_off_z = (float)(player_tile_z + tz) * floor_tile_span_z;
                            g_floor_center_x = tile_off_x;
                            g_floor_center_z = tile_off_z;
                            build_floor_tile(tile_off_x, tile_off_z);
                            g_path1_mesh_stage = "floor_tile";
                            q = render_path1_tris(q, mvp, floor_tile_tris, path1_vert_count, path1_vert_count);
                            q = add_dma_tag(q, 0, 7, 0, 0, 0);
                            FlushCache(0);
                            path1_vif_packet_submit(vif_pkt, q, dbg_frame, "floor_tile");
                            tile_submits++;
                        }
                        if (tile_submits >= tile_budget) break;
                    }
                } else {
                    packet_t *vif_pkt = path1_vif_packet_begin();
                    q = vif_pkt->data;
                    g_floor_center_x = 0.0f;
                    g_floor_center_z = 0.0f;
                    g_path1_mesh_stage = "floor_single";
#if PANTHEON_FLOOR_PATH1_DOUBLE_SIDED
                    build_floor_tile(0.0f, 0.0f);
#endif
                    q = render_floor_path1_count(q, mvp, path1_vert_count);
                    q = add_dma_tag(q, 0, 7, 0, 0, 0);
                    FlushCache(0);
                    path1_vif_packet_submit(vif_pkt, q, dbg_frame, "floor_single");
                    tile_submits = 1;
                }
#endif
                g_last_tile_submits = tile_submits;
                #if PANTHEON_MIN_TELEMETRY
                if ((dbg_frame % 120) == 0) {
                    printf("pantheon path1 frame=%d verts=%d vif_qw=%d nearz_hits=%d tiles=%d cam=(%.2f,%.2f,%.2f) rot=(%.3f,%.3f) near=(%.2f/%.2f)\n",
                           dbg_frame,
                           g_last_batch_verts,
                           g_last_vif1_qw,
                           g_last_nearz_hits,
                           g_last_tile_submits,
                           camera_position[0], camera_position[1], camera_position[2],
                           camera_rotation[0], camera_rotation[1],
                           PANTHEON_VIEW_NEAR,
                           PANTHEON_VU_NEARZ);
                }
                #endif
            }
        }

        graph_wait_vsync();
        graph_set_framebuffer(0, frame[context].address, frame[context].width, frame[context].psm, 0, 0);
        context = !context;
    }
    return 0;
}