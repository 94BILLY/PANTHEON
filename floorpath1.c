/*===========================================================================
 * PANTHEON ENGINE — production Path 1 (VU1 → XGKICK → GIF → GS)
 *
 * All *geometry* (skydome, floor, world) goes through VU1: EE builds
 * 128-bit aligned DMA/VIF1 chains, microcode transforms, XGKICK.
 *
 * A minimal GIF (Path 2) chain is used only for GS state + framebuffer clear
 * (no libdraw draw_prim triangle strips for scene meshes — that is debug-only).
 * Softimage 3D → hrcConvert (NT) → ASCII .hrc → hrc2ps2.py → headers / embed.
 *===========================================================================*/

#include <kernel.h>
#include <stdlib.h>
#include <malloc.h>
#include <tamtypes.h>
#include <math3d.h>
#include <packet.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "skydome_data.h"

/* 6x6 cell floor (7x7 verts), Bliss-style checker — Softimage 6x6 grid topology.
 * Filled in init_flat_floor() for Path 1; also drives optional GIF debug overlay. */
#define FLOOR_DIV 6u
#define FLOOR_LINE ((FLOOR_DIV) + 1u)
#define FLOOR_VERT_COUNT (FLOOR_LINE * FLOOR_LINE) /* 49 */
#define FLOOR_TRI_COUNT (FLOOR_DIV * FLOOR_DIV * 2u) /* 72 */
static PantheonVertex floor_vertices[FLOOR_VERT_COUNT];
static PantheonTriangle floor_indices[FLOOR_TRI_COUNT];

/* Authored Softimage skydome in VU1 when cruncher produced tris (else CPU placeholder). */
#define USE_VU1_SKYDOME_MESH (SKYDOME_TRI_COUNT > 0)

#ifndef PANTHEON_DEBUG_LOG
#define PANTHEON_DEBUG_LOG 0
#endif

// ==================== GTA SAN ANDREAS CAMERA ====================
static char padBuf[256] __attribute__((aligned(64)));

static float player_x = 0.0f, player_y = 0.0f, player_z = 0.0f;
static float player_yaw = 0.0f;
static const float CAM_DIST = 250.0f;   // pulled back more

static VECTOR camera_position = {0.0f, 180.0f, 250.0f, 1.0f};
static VECTOR camera_rotation = {-0.6f, 0.0f, 0.0f, 1.0f};

static const float PAD_DEADZONE = 24.0f;
static const float RIGHT_YAW_SENS = 0.00135f;
static const float RIGHT_PITCH_SENS = 0.00095f;
static const float MOVE_SPEED = 2.15f;
static const float PI_F = 3.14159265f;

static float cam_yaw = 0.0f;
static float cam_pitch = 0.6f;
static float target_yaw = 0.5f;
static float target_pitch = 0.5f;
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
/* 0 = shipping: no CPU/GIF draw_prim mesh submission (MGS2/ND/SA style scene path). */
#define PANTHEON_GIF_DEBUG_MESH 0
#if PANTHEON_GIF_DEBUG_MESH
#define SHOW_CPU_PROBE 1
#define SHOW_CPU_EXPORTED_FLOOR 1
#define SHOW_SKY_PLACEHOLDER 1
#else
#define SHOW_CPU_PROBE 0
#define SHOW_CPU_EXPORTED_FLOOR 0
#define SHOW_SKY_PLACEHOLDER 0
#endif

/* Must match the scale applied in init_flat_floor() to floor mesh (Softimage export). */
#define FLOOR_MESH_SCALE 6.0f

#if USE_VU1_SKYDOME_MESH
/* Scale Softimage mesh units (~radius 5 sphere) into world space. */
#define SKYDOME_MESH_SCALE 320.0f
static PantheonVertex flat_skydome[SKYDOME_TRI_COUNT * 3] __attribute__((aligned(16)));
static int flat_skydome_vert_count = 0;
#endif

packet_t *packets[2];
int context = 0;
static unsigned int debug_frame_counter = 0;

// Forward declarations
qword_t *render_clear_and_setup(qword_t *q, int ctx, framebuffer_t *frame, zbuffer_t *z);
qword_t *render_floor_path1_count(qword_t *q, MATRIX mvp, int total_verts);
static qword_t *render_path1_tris(qword_t *q, MATRIX mvp, const PantheonVertex *mesh_tris, int total_verts, int max_verts);
#if PANTHEON_GIF_DEBUG_MESH && !USE_VU1_SKYDOME_MESH
static qword_t *render_sky_placeholder_dome(qword_t *q, MATRIX world_view, MATRIX view_screen);
#endif
static void clamp_player_to_floor(void);

#if PANTHEON_DEBUG_LOG
static void debug_log(const char *run_id, const char *hypothesis_id, const char *location, const char *message, const char *data_json) {
    FILE *f = fopen("host:debug-a495e9.log", "a");
    if (!f)
        f = fopen("debug-a495e9.log", "a");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"pantheon\",\"runId\":\"%s\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\",\"data\":%s,\"timestamp\":0}\n",
        run_id, hypothesis_id, location, message, data_json
    );
    fclose(f);
}
#else
#define debug_log(run_id, hypothesis_id, location, message, data_json) ((void)0)
#endif

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
    if (!pad_ready) return;

    struct padButtonStatus buttons;
    
    // Pre-center sticks in case the read fails or analog isn't fully on yet
    buttons.ljoy_h = 128;
    buttons.ljoy_v = 128;
    buttons.rjoy_h = 128;
    buttons.rjoy_v = 128;

    int read_ok = padRead(0, 0, &buttons);
    if (read_ok == 0) {
        // #region agent log
        debug_log("run1", "H2", "floor.c:read_pad_analog", "padRead returned 0", "{\"pad_ready\":1}");
        // #endregion
        return;
    }

    float lx = (float)buttons.ljoy_h - 127.5f;
    float ly = (float)buttons.ljoy_v - 127.5f;
    float rx = (float)buttons.rjoy_h - 127.5f;
    float ry = (float)buttons.rjoy_v - 127.5f;

    if (fabsf(lx) < PAD_DEADZONE) lx = 0.0f;
    if (fabsf(ly) < PAD_DEADZONE) ly = 0.0f;
    if (fabsf(rx) < PAD_DEADZONE) rx = 0.0f;
    if (fabsf(ry) < PAD_DEADZONE) ry = 0.0f;

    /* D-pad: must work even when the left stick is slightly off-center (raw != 0). */
    u32 paddata = 0xffffu ^ (u32)buttons.btns;
    int dpx = 0, dpy = 0;
    if (paddata & PAD_LEFT)  dpx--;
    if (paddata & PAD_RIGHT) dpx++;
    if (paddata & PAD_UP)    dpy--;
    if (paddata & PAD_DOWN)  dpy++;

    float mvx, mvy;
    if (dpx != 0 || dpy != 0) {
        mvx = (float)dpx * 127.5f;
        mvy = (float)dpy * 127.5f;
    } else {
        mvx = lx;
        mvy = ly;
    }

    /* "Up" pulls the ground toward the player (negate Y). Invert left/right on stick + D-pad (do not negate X). */
    mvy = -mvy;

    if ((debug_frame_counter % 120) == 0) {
        char buf[256];
        snprintf(
            buf, sizeof(buf),
            "{\"ljoy_h\":%u,\"ljoy_v\":%u,\"rjoy_h\":%u,\"rjoy_v\":%u,\"mvx\":%.2f,\"mvy\":%.2f,\"rx\":%.2f,\"ry\":%.2f}",
            (unsigned int)buttons.ljoy_h, (unsigned int)buttons.ljoy_v,
            (unsigned int)buttons.rjoy_h, (unsigned int)buttons.rjoy_v,
            mvx, mvy, rx, ry
        );
        // #region agent log
        debug_log("run1", "H3", "floor.c:read_pad_analog", "sampled analog values", buf);
        // #endregion
    }

    target_yaw   += rx * RIGHT_YAW_SENS;
    target_pitch += ry * RIGHT_PITCH_SENS;
    target_yaw = wrap_angle_pi(target_yaw);

    if (target_pitch < 0.08f)  target_pitch = 0.08f;
    if (target_pitch > 1.32f)  target_pitch = 1.32f;

    cam_yaw   += (target_yaw   - cam_yaw)   * 0.165f;
    cam_pitch += (target_pitch - cam_pitch) * 0.155f;
    cam_yaw = wrap_angle_pi(cam_yaw);

    // GTA-style camera-relative movement with diagonal normalization.
    float mag = sqrtf((mvx * mvx) + (mvy * mvy));
    if (mag > 0.0f) {
        float nx = mvx / mag;
        float ny = mvy / mag;
        float move_scale = MOVE_SPEED * (mag / 127.5f);

        float move_x = (sinf(cam_yaw) * -ny) + (cosf(cam_yaw) * nx);
        float move_z = (cosf(cam_yaw) * -ny) - (sinf(cam_yaw) * nx);

        player_x += move_x * move_scale;
        player_z += move_z * move_scale;
        player_yaw = wrap_angle_pi(atan2f(nx, -ny) + cam_yaw);

        if ((debug_frame_counter % 120) == 0) {
            char buf[256];
            snprintf(
                buf, sizeof(buf),
                "{\"mag\":%.3f,\"move_scale\":%.3f,\"move_x\":%.3f,\"move_z\":%.3f,\"player_x\":%.3f,\"player_z\":%.3f}",
                mag, move_scale, move_x, move_z, player_x, player_z
            );
            // #region agent log
            debug_log("run1", "H4", "floor.c:read_pad_analog", "movement applied", buf);
            // #endregion
        }
    }

    clamp_player_to_floor();
}

static void update_camera_orbit(void) {
    float cos_p = cosf(cam_pitch);
    float sin_p = sinf(cam_pitch);

    camera_position[0] = player_x + (sinf(cam_yaw) * cos_p * CAM_DIST);
    camera_position[1] = player_y + (sin_p * CAM_DIST) + 60.0f;
    camera_position[2] = player_z + (cosf(cam_yaw) * cos_p * CAM_DIST);
    camera_position[3] = 1.0f;

    camera_rotation[0] = -cam_pitch;
    camera_rotation[1] = -cam_yaw;
    camera_rotation[2] = 0.0f;
    camera_rotation[3] = 1.0f;
}

/* Flattened triangle stream for VU1 (CPU expand at boot; see flight log Day 4). */
PantheonVertex flat_floor[FLOOR_TRI_COUNT * 3] __attribute__((aligned(16)));

/* Walk bounds: XZ AABB of scaled floor mesh. */
static float floor_bound_x0, floor_bound_x1, floor_bound_z0, floor_bound_z1;

static void init_floor_walk_bounds(void) {
    float minx = flat_floor[0].x;
    float maxx = minx;
    float minz = flat_floor[0].z;
    float maxz = minz;
    int n = FLOOR_TRI_COUNT * 3;
    for (int i = 1; i < n; i++) {
        float x = flat_floor[i].x;
        float z = flat_floor[i].z;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (z < minz) minz = z;
        if (z > maxz) maxz = z;
    }
    floor_bound_x0 = minx;
    floor_bound_x1 = maxx;
    floor_bound_z0 = minz;
    floor_bound_z1 = maxz;
}

static void clamp_player_to_floor(void) {
    if (player_x < floor_bound_x0) player_x = floor_bound_x0;
    if (player_x > floor_bound_x1) player_x = floor_bound_x1;
    if (player_z < floor_bound_z0) player_z = floor_bound_z0;
    if (player_z > floor_bound_z1) player_z = floor_bound_z1;
}

#if !USE_VU1_SKYDOME_MESH && PANTHEON_GIF_DEBUG_MESH
/* Procedural sky hemisphere (GIF / Path 2 debug) when no VU1 skydome. */
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
#endif /* !USE_VU1_SKYDOME_MESH && PANTHEON_GIF_DEBUG_MESH */

#define PHYSICAL(addr) ((u32)(addr) & 0x0FFFFFFF)

/* GS RGBAQ slot X: packed u8 RGBA as one float (same layout as init_flat_floor). */
static inline float pantheon_rgbaq_from_u8(u8 r, u8 g, u8 b, u8 a) {
    union {
        u32 i;
        float f;
    } pun;
    pun.i = ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
    return pun.f;
}

static inline qword_t* add_dma_tag(qword_t *q, u16 qwc, u8 id, u32 addr, u32 vif0, u32 vif1) {
    q->sw[0] = (qwc & 0xFFFF) | ((id & 0x7) << 28);
    q->sw[1] = PHYSICAL(addr);
    q->sw[2] = vif0;
    q->sw[3] = vif1;
    return q + 1;
} 

/* 6x6 grid (7x7 verts), checker grass; expand to flat tri strip for VU1. */
void init_flat_floor(void) {
    const float h = 50.0f; /* pre-scale half-extent (× FLOOR_MESH_SCALE → ±300) */
    const float step = (2.0f * h) / (float)FLOOR_DIV;
    unsigned int vi = 0;
    for (unsigned int j = 0; j < FLOOR_LINE; j++) {
        for (unsigned int i = 0; i < FLOOR_LINE; i++) {
            PantheonVertex *v = &floor_vertices[vi];
            v->x = -h + (float)i * step;
            v->y = 0.0f;
            v->z = -h + (float)j * step;
            v->w = 1.0f;
            /* Gouraud-style vertex tint for optional GIF debug overlay */
            int ck = (int)((i + j) & 1u);
            v->r = ck ? 0.22f : 0.14f;
            v->g = ck ? 0.58f : 0.45f;
            v->b = ck ? 0.14f : 0.10f;
            v->a = 1.0f;
            vi++;
        }
    }
    int ti = 0;
    for (unsigned int j = 0; j < FLOOR_DIV; j++) {
        for (unsigned int i = 0; i < FLOOR_DIV; i++) {
            unsigned int tli = j * FLOOR_LINE + i;
            unsigned int tri_ = tli + 1u;
            unsigned int bli = (j + 1u) * FLOOR_LINE + i;
            unsigned int bri = bli + 1u;
            floor_indices[ti].a = tli;
            floor_indices[ti].b = tri_;
            floor_indices[ti].c = bli;
            floor_indices[ti].pad = 0;
            ti++;
            floor_indices[ti].a = tri_;
            floor_indices[ti].b = bri;
            floor_indices[ti].c = bli;
            floor_indices[ti].pad = 0;
            ti++;
        }
    }
    /* Two packed tri colors (checker) for Path 1 flat RGBAQ */
    union {
        u32 i;
        float f;
    } dark, light;
    dark.i  = (128u << 24) | (50u  << 16) | (30u  << 8) | 20u;
    light.i = (128u << 24) | (60u  << 16) | (200u << 8) | 48u;

    int out_idx = 0;
    for (int t = 0; t < FLOOR_TRI_COUNT; t++) {
        int cell = t / 2;
        int cj = cell / (int)FLOOR_DIV;
        int ci = cell % (int)FLOOR_DIV;
        float packc = ((ci + cj) & 1) ? light.f : dark.f;
        int ia = (int)floor_indices[t].a;
        int ib = (int)floor_indices[t].b;
        int ic = (int)floor_indices[t].c;
        flat_floor[out_idx] = floor_vertices[ia];
        flat_floor[out_idx].r = packc;
        flat_floor[out_idx].g = 1.0f;
        flat_floor[out_idx].b = 0.0f;
        flat_floor[out_idx].a = 0.0f;
        out_idx++;
        flat_floor[out_idx] = floor_vertices[ib];
        flat_floor[out_idx].r = packc;
        flat_floor[out_idx].g = 1.0f;
        flat_floor[out_idx].b = 0.0f;
        flat_floor[out_idx].a = 0.0f;
        out_idx++;
        flat_floor[out_idx] = floor_vertices[ic];
        flat_floor[out_idx].r = packc;
        flat_floor[out_idx].g = 1.0f;
        flat_floor[out_idx].b = 0.0f;
        flat_floor[out_idx].a = 0.0f;
        out_idx++;
    }
    for (int i = 0; i < FLOOR_TRI_COUNT * 3; i++) {
        flat_floor[i].x *= FLOOR_MESH_SCALE;
        flat_floor[i].y *= FLOOR_MESH_SCALE;
        flat_floor[i].z *= FLOOR_MESH_SCALE;
    }
}

#if USE_VU1_SKYDOME_MESH
static void init_flat_skydome(void) {
    int out_idx = 0;
    for (int i = 0; i < SKYDOME_TRI_COUNT; i++) {
        const PantheonVertex *va = &skydome_vertices[skydome_indices[i].a];
        const PantheonVertex *vb = &skydome_vertices[skydome_indices[i].b];
        const PantheonVertex *vc = &skydome_vertices[skydome_indices[i].c];
        /* Runtime hemisphere cut: keep only upper hemisphere from full sphere source. */
        if (va->y < 0.0f || vb->y < 0.0f || vc->y < 0.0f)
            continue;

        /* Inward winding: viewer sits under the dome; Softimage outward normals need
         * (a,c,b) so GS front-faces point inward. Re-export an upper hemisphere only
         * from Softimage (y >= 0 cap) to save tris — same winding rule applies. */
        flat_skydome[out_idx++] = *va;
        flat_skydome[out_idx++] = *vc;
        flat_skydome[out_idx++] = *vb;
    }
    /* If cull removed everything, fall back to full sphere (still run winding fix). */
    if (out_idx == 0) {
        for (int i = 0; i < SKYDOME_TRI_COUNT; i++) {
            flat_skydome[out_idx++] = skydome_vertices[skydome_indices[i].a];
            flat_skydome[out_idx++] = skydome_vertices[skydome_indices[i].c];
            flat_skydome[out_idx++] = skydome_vertices[skydome_indices[i].b];
        }
    }
    flat_skydome_vert_count = out_idx;

    for (int i = 0; i < flat_skydome_vert_count; i++) {
        PantheonVertex *v = &flat_skydome[i];
        v->x *= SKYDOME_MESH_SCALE;
        v->y *= SKYDOME_MESH_SCALE;
        v->z *= SKYDOME_MESH_SCALE;

        float len2 = v->x * v->x + v->y * v->y + v->z * v->z;
        float invl = (len2 > 1e-8f) ? (1.0f / sqrtf(len2)) : 0.0f;
        float ny = v->y * invl;
        /* 0 = horizon ring, 1 = zenith (+Y in typical Softimage sphere export). */
        float t = 0.5f * (ny + 1.0f);
        if (t < 0.0f)
            t = 0.0f;
        if (t > 1.0f)
            t = 1.0f;

        u8 r_z = 28, g_z = 105, b_z = 210;
        u8 r_h = 175, g_h = 215, b_h = 248;
        float tf = 1.0f - t;
        u8 r = (u8)(tf * (float)r_h + t * (float)r_z);
        u8 g = (u8)(tf * (float)g_h + t * (float)g_z);
        u8 b = (u8)(tf * (float)b_h + t * (float)b_z);

        float puff = sinf(v->x * 0.0045f + v->z * 0.0055f + v->y * 0.0025f);
        if (puff > 0.58f) {
            float w = (puff - 0.58f) * 2.2f;
            if (w > 0.7f)
                w = 0.7f;
            r = (u8)fminf(255.0f, (float)r + w * 78.0f);
            g = (u8)fminf(255.0f, (float)g + w * 72.0f);
            b = (u8)fminf(255.0f, (float)b + w * 38.0f);
        }

        v->r = pantheon_rgbaq_from_u8(r, g, b, 255);
        v->g = 1.0f;
        v->b = 0.0f;
        v->a = 0.0f;
    }
}
#endif

static qword_t *render_path1_tris(qword_t *q, MATRIX mvp, const PantheonVertex *mesh_tris, int total_verts, int max_verts) {
    u32 u32_count = (u32)(&PantheonShaderEnd - &PantheonShader);
    u32 shader_qwc = u32_count / 4;
    u32 shader_instr = u32_count / 2;

    q = add_dma_tag(q, shader_qwc, 3, (u32)&PantheonShader, 0, VIF_CODE(P_VIF_MPG, shader_instr, 0));

    q = add_dma_tag(q, 1, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 1, 0));
    {
        float *f = (float *)q->sw;
        f[0] = 0.0f; f[1] = 0.0f; f[2] = 0.0f; f[3] = 1.0f;
    }
    q++;

    if (total_verts > max_verts) total_verts = max_verts;
    if (total_verts <= 0) return q;
    int verts_per_batch = 108;

    for (int i = 0; i < total_verts; i += verts_per_batch) {
        int verts_this_batch = total_verts - i;
        if (verts_this_batch > verts_per_batch) verts_this_batch = verts_per_batch;

        q = add_dma_tag(q, 1, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 1, 1));
        q->dw[0] = GIF_SET_TAG(verts_this_batch, 1, 1, GIF_PRIM_TRIANGLE | 8, GIF_FLG_PACKED, 2);
        q->dw[1] = GIF_REG_RGBAQ | ((u64)GIF_REG_XYZ2 << 4); q++;

        q = add_dma_tag(q, 1, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 1, 2));
        float *f = (float *)q->sw;
        f[0] = 2048.0f; f[1] = 2048.0f; f[2] = 8388608.0f; f[3] = 1.0f;
        q++;

        q = add_dma_tag(q, 4, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, 4, 3));
        memcpy(q, mvp, 64); q += 4;

        int batch_qwc = verts_this_batch * 2;
        q = add_dma_tag(q, batch_qwc, 1, 0, 0, VIF_CODE(P_VIF_UNPACK | 0x0c, batch_qwc, 7));
        memcpy(q, &mesh_tris[i], batch_qwc * 16); q += batch_qwc;

        int itops_val = verts_this_batch * 2;

        q = add_dma_tag(q, 0, 1, 0,
        VIF_CODE(0x04, 0, itops_val),
        VIF_CODE(P_VIF_MSCAL, 0, 0));

        q = add_dma_tag(q, 0, 1, 0,
        VIF_CODE(P_VIF_FLUSHE, 0, 0),
        0);
    }

    return q;
}

qword_t *render_floor_path1_count(qword_t *q, MATRIX mvp, int total_verts) {
    return render_path1_tris(q, mvp, flat_floor, total_verts, FLOOR_TRI_COUNT * 3);
}

qword_t *render_clear_and_setup(qword_t *q, int ctx, framebuffer_t *frame, zbuffer_t *z) {
    q = draw_setup_environment(q, 0, &frame[ctx], z);
    q = draw_primitive_xyoffset(q, 0, 2048 - 320, 2048 - 224);
    /* Clear to horizon blue (VU1 skydome draws over this; no GIF mesh in shipping). */
    q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 224.0f, 640.0f, 448.0f, 105, 155, 215);
    return q;
}

#if PANTHEON_GIF_DEBUG_MESH
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
            floor_positions[i][0] = floor_vertices[i].x * FLOOR_MESH_SCALE;
            floor_positions[i][1] = floor_vertices[i].y * FLOOR_MESH_SCALE;
            floor_positions[i][2] = floor_vertices[i].z * FLOOR_MESH_SCALE;
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
        q->dw[0] = col[ia].rgbaq; q->dw[1] = xyz[ia].xyz; q++;
        q->dw[0] = col[ib].rgbaq; q->dw[1] = xyz[ib].xyz; q++;
        q->dw[0] = col[ic].rgbaq; q->dw[1] = xyz[ic].xyz; q++;
    }
    q = draw_prim_end(q, 2, DRAW_RGBAQ_REGLIST);
    return q;
}
#endif /* PANTHEON_GIF_DEBUG_MESH */

// ==================== MAIN ====================

int main(int argc, char *argv[]) {
    SifInitRpc(0);

    // Load IOP drivers
    int sio2_res = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    int padman_res = SifLoadModule("rom0:PADMAN", 0, NULL);
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"sio2_res\":%d,\"padman_res\":%d}", sio2_res, padman_res);
        // #region agent log
        debug_log("run1", "H1", "floor.c:main", "pad modules loaded", buf);
        // #endregion
    }

    if (sio2_res >= 0 && padman_res >= 0) {
        padInit(0);
        if (padPortOpen(0, 0, padBuf) != 0) {
            pad_port_open_ok = 1;
        }
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "{\"pad_port_open_ok\":%d}", pad_port_open_ok);
            // #region agent log
            debug_log("run1", "H1", "floor.c:main", "pad port open result", buf);
            // #endregion
        }
    }

    // Setup framebuffers and Z-buffer
    framebuffer_t frame[2]; 
    zbuffer_t z;

    frame[0].width = 640; frame[0].height = 448; frame[0].psm = GS_PSM_32;
    frame[0].address = graph_vram_allocate(640, 448, GS_PSM_32, GRAPH_ALIGN_PAGE);
    frame[1].width = 640; frame[1].height = 448; frame[1].psm = GS_PSM_32;
    frame[1].address = graph_vram_allocate(640, 448, GS_PSM_32, GRAPH_ALIGN_PAGE);

    z.enable = DRAW_ENABLE; z.mask = 0; z.method = ZTEST_METHOD_ALLPASS; z.zsm = GS_PSMZ_32;
    z.address = graph_vram_allocate(640, 448, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

    graph_initialize(frame[0].address, frame[0].width, frame[0].height, frame[0].psm, 0, 0);
    graph_set_framebuffer(0, frame[0].address, frame[0].width, frame[0].psm, 0, 0);
    graph_enable_output(); 

    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);

    /* Large enough for VU1 DMA (shader + many UNPACK batches); 4k was too small → only clear visible. */
    packets[0] = packet_init(32000, PACKET_NORMAL);
    packets[1] = packet_init(32000, PACKET_NORMAL);

    init_flat_floor();
    init_floor_walk_bounds();
#if USE_VU1_SKYDOME_MESH
    init_flat_skydome();
#endif
#if PANTHEON_GIF_DEBUG_MESH && !USE_VU1_SKYDOME_MESH
    init_sky_placeholder_dome();
#endif

    MATRIX world_view, view_screen, mvp;
    create_view_screen(view_screen, graph_aspect_ratio(), -3.0f, 3.0f, -3.0f, 3.0f, 1.0f, 2000.0f);

    while (1) {
        debug_frame_counter++;
        // --- STEP 1: INPUT & CAMERA ---
        try_finish_pad_init();
        read_pad_analog();
        update_camera_orbit();
        if ((debug_frame_counter % 120) == 0) {
            char buf[256];
            snprintf(
                buf, sizeof(buf),
                "{\"pad_ready\":%d,\"cam_yaw\":%.3f,\"cam_pitch\":%.3f,\"camera_x\":%.3f,\"camera_y\":%.3f,\"camera_z\":%.3f,\"player_x\":%.3f,\"player_z\":%.3f}",
                pad_ready, cam_yaw, cam_pitch,
                camera_position[0], camera_position[1], camera_position[2],
                player_x, player_z
            );
            // #region agent log
            debug_log("run1", "H5", "floor.c:main_loop", "camera/frame state sample", buf);
            // #endregion
        }

        create_world_view(world_view, camera_position, camera_rotation);
        matrix_multiply(mvp, world_view, view_screen);

        /* GS setup + clear (GIF register writes only — no scene triangles here). */
        packet_reset(packets[context]);
        qword_t *q = packets[context]->data;

        q = render_clear_and_setup(q, context, frame, &z);
#if PANTHEON_GIF_DEBUG_MESH
#if !USE_VU1_SKYDOME_MESH
        if (SHOW_SKY_PLACEHOLDER) {
            q = draw_disable_tests(q, 0, &z);
            q = render_sky_placeholder_dome(q, world_view, view_screen);
            q = draw_enable_tests(q, 0, &z);
        }
#endif
        if (SHOW_CPU_PROBE) {
            q = render_cpu_probe(q, world_view, view_screen);
        }
        if (SHOW_CPU_EXPORTED_FLOOR) {
            q = render_cpu_exported_floor(q, world_view, view_screen);
        }
#endif
        
        FlushCache(0); 
        dma_channel_send_normal(DMA_CHANNEL_GIF, (void *)PHYSICAL(packets[context]->data), q - packets[context]->data, 0, 0);
        dma_wait_fast(); // Fixed: No arguments
        /* Do not block on FINISH here: this packet path does not guarantee a FINISH event,
         * and waiting for it can stall the main loop (FPS becomes N/A, input appears dead). */

        // --- VU1 Path 1: all scene geometry (skydome, then floor) ---
        if (RENDER_STAGE >= 2) {
#if USE_VU1_SKYDOME_MESH
            /* Skydome first (same VU1 shader + packed RGBAQ). */
            packet_reset(packets[context]);
            q = packets[context]->data;
            q = render_path1_tris(q, mvp, flat_skydome, flat_skydome_vert_count, flat_skydome_vert_count);
            q = add_dma_tag(q, 0, 7, 0, 0, 0);
            FlushCache(0);
            {
                int vif1_qw = (int)(q - packets[context]->data);
                dma_channel_send_chain(DMA_CHANNEL_VIF1, (void *)PHYSICAL(packets[context]->data), vif1_qw, 0, 0);
            }
            dma_wait_fast();
#endif
            packet_reset(packets[context]);
            q = packets[context]->data;

            int path1_vert_count = (FLOOR_TRI_COUNT * 3);
            q = render_floor_path1_count(q, mvp, path1_vert_count);

            // End the DMA chain properly
            q = add_dma_tag(q, 0, 7, 0, 0, 0);

            FlushCache(0);
            {
                int vif1_qw = (int)(q - packets[context]->data);
                dma_channel_send_chain(DMA_CHANNEL_VIF1, (void *)PHYSICAL(packets[context]->data), vif1_qw, 0, 0);
            }
            dma_wait_fast();
        }

        // --- STEP 4: FLIP ---
        graph_wait_vsync();
        graph_set_framebuffer(0, frame[context].address, frame[context].width, frame[context].psm, 0, 0);
        context = !context;
    }

    return 0;
}