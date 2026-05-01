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
#include <draw3d.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "floor_data.h"
#include "skydome_data.h"
#include "pantheon_path1_contract.h"
#include "pantheon_timecycle.h"
#include "pantheon_vram.h"
#include "pantheon_texture_host.h"

#ifndef PANTHEON_TEXTURE_PHASE
#define PANTHEON_TEXTURE_PHASE 0
#endif

#ifndef PANTHEON_SKY_CLOUD_PUFF
#define PANTHEON_SKY_CLOUD_PUFF 0
#endif

/* Authored Softimage skydome in VU1 when cruncher produced tris (else CPU placeholder). */
#define USE_VU1_SKYDOME_MESH (SKYDOME_TRI_COUNT > 0)

#ifndef PANTHEON_DEBUG_LOG
#define PANTHEON_DEBUG_LOG 0
#endif

#ifndef PANTHEON_AGENT_TRACE
#define PANTHEON_AGENT_TRACE 0
#endif

// ==================== GTA SAN ANDREAS CAMERA ====================
static char padBuf[256] __attribute__((aligned(64)));

static float player_x = 0.0f, player_y = 0.0f, player_z = 0.0f;
static float player_yaw = 0.0f;
#ifndef PANTHEON_CAM_DIST
#define PANTHEON_CAM_DIST 180.0f
#endif
#ifndef PANTHEON_CAMERA_HEIGHT_OFFSET
#define PANTHEON_CAMERA_HEIGHT_OFFSET 20.0f
#endif
#ifndef PANTHEON_FIRST_PERSON_VIEW
#define PANTHEON_FIRST_PERSON_VIEW 0
#endif
#ifndef PANTHEON_FIRST_PERSON_EYE_HEIGHT
#define PANTHEON_FIRST_PERSON_EYE_HEIGHT 6.0f
#endif
static const float CAM_DIST = PANTHEON_CAM_DIST;

static VECTOR camera_position = {0.0f, 106.0f, 158.0f, 1.0f};
static VECTOR camera_rotation = {-0.6f, 0.0f, 0.0f, 1.0f};

static const float PAD_DEADZONE = 24.0f;
static const float RIGHT_YAW_SENS = 0.00135f;
static const float RIGHT_PITCH_SENS = 0.00095f;
static const float MOVE_SPEED = 2.15f;
static const float PI_F = 3.14159265f;

static float cam_yaw = 0.0f;
static float cam_pitch = 0.10f;
static float target_yaw = 0.5f;
static float target_pitch = 0.10f;
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
#define PANTHEON_TRIAGE_STATIC_CAMERA 0
#endif

#ifndef PANTHEON_TRIAGE_ENABLE_SKYDOME
#define PANTHEON_TRIAGE_ENABLE_SKYDOME 1
#endif

#ifndef PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER
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
#define PANTHEON_TRIAGE_PITCH_SIGN_FLIP 1
#endif

#ifndef PANTHEON_CAMERA_ENVELOPE_HARDEN
#define PANTHEON_CAMERA_ENVELOPE_HARDEN 1
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
#define PANTHEON_VU_NEARZ 0.1f
#endif
#ifndef PANTHEON_PATH1_GIF_USE_ST
#define PANTHEON_PATH1_GIF_USE_ST 1
#endif
#ifndef PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
#define PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE 1
#endif

#ifndef PANTHEON_TILE_RADIUS
#define PANTHEON_TILE_RADIUS 1
#endif

#ifndef PANTHEON_TILE_SUBMIT_BUDGET
#define PANTHEON_TILE_SUBMIT_BUDGET 9
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
static int g_dbg_frame = 0;
static int g_last_nearz_hits = 0;
static int g_last_batch_verts = 0;
static int g_last_vif1_qw = 0;
static int g_last_tile_submits = 0;
static const char *g_path1_mesh_stage = "unknown";
static float g_floor_center_x = 0.0f;
static float g_floor_center_z = 0.0f;
static PantheonAtmosphere g_atmosphere;
static PantheonWeather g_weather = PANTHEON_WEATHER_CLEAR;
static float g_day01 = 0.58f;
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
#define PANTHEON_DAY_CYCLE_SECONDS 600.0f
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
qword_t *render_clear_and_setup(qword_t *q, int ctx, framebuffer_t *frame, zbuffer_t *z);
qword_t *render_floor_path1_count(qword_t *q, MATRIX mvp, int total_verts);
static qword_t *render_path1_tris(qword_t *q, MATRIX mvp, const PantheonVertex *mesh_tris, int total_verts, int max_verts);
static void update_atmosphere(int frame_id);
static void update_skydome_colors(void);
#if !USE_VU1_SKYDOME_MESH
static qword_t *render_sky_placeholder_dome(qword_t *q, MATRIX world_view, MATRIX view_screen);
#endif
static void clamp_player_to_floor(void);
static void apply_sky_color(PantheonVertex *v);

static FILE *pantheon_log_stream_open(const char *primary, const char *fallback1, const char *fallback2, const char *fallback3) {
    static FILE *dbg22_stream = NULL;
    static FILE *debug_stream = NULL;
    static FILE *f266a2_stream = NULL;
    FILE **slot = NULL;

    if (strcmp(primary, "host:/home/marvin/Pantheon/.cursor/debug-22f5dd.log") == 0) {
        slot = &dbg22_stream;
    } else if (strcmp(primary, "host:debug-a495e9.log") == 0) {
        slot = &debug_stream;
    } else if (strcmp(primary, "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log") == 0) {
        slot = &f266a2_stream;
    } else {
        return NULL;
    }
    if (*slot) {
        return *slot;
    }

    *slot = fopen(primary, "a");
    if (!*slot && fallback1) *slot = fopen(fallback1, "a");
    if (!*slot && fallback2) *slot = fopen(fallback2, "a");
    if (!*slot && fallback3) *slot = fopen(fallback3, "a");
    if (*slot) {
        setvbuf(*slot, NULL, _IOLBF, 0);
    }
    return *slot;
}

static void pantheon_log_flush_maybe(FILE *f) {
    static int log_lines = 0;
    if (!f) {
        return;
    }
    log_lines++;
    if ((log_lines % 60) == 0) {
        fflush(f);
    }
}

static void dbg22_log_json(const char *run_id, const char *hypothesis_id, const char *location, const char *message, const char *data_json) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-22f5dd.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-22f5dd.log",
        "host:.cursor/debug-22f5dd.log",
        "host:debug-22f5dd.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"22f5dd\",\"runId\":\"%s\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\",\"data\":%s,\"timestamp\":0}\n",
        run_id, hypothesis_id, location, message, data_json
    );
    pantheon_log_flush_maybe(f);
}

#if PANTHEON_DEBUG_LOG
static void debug_log(const char *run_id, const char *hypothesis_id, const char *location, const char *message, const char *data_json) {
    FILE *f = pantheon_log_stream_open("host:debug-a495e9.log", "debug-a495e9.log", NULL, NULL);
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"pantheon\",\"runId\":\"%s\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\",\"data\":%s,\"timestamp\":0}\n",
        run_id, hypothesis_id, location, message, data_json
    );
    pantheon_log_flush_maybe(f);
}
#else
#define debug_log(run_id, hypothesis_id, location, message, data_json) ((void)0)
#endif

#if PANTHEON_AGENT_TRACE
// #region agent log
static void agent_dbg22_log(const char *run_id, const char *hypothesis_id, const char *location, const char *message, const char *data_json) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-22f5dd.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-22f5dd.log",
        "host:.cursor/debug-22f5dd.log",
        "host:debug-22f5dd.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"22f5dd\",\"runId\":\"%s\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\",\"data\":%s,\"timestamp\":0}\n",
        run_id, hypothesis_id, location, message, data_json
    );
    pantheon_log_flush_maybe(f);
}
// #endregion
#else
#define agent_dbg22_log(run_id, hypothesis_id, location, message, data_json) ((void)0)
#endif

#if PANTHEON_AGENT_TRACE
/* H1: dma_channel_send_chain(..., qwc, ...) with qwc==0 transfers no EE data → VIF1 idle → no geometry. */
static void agent_path1_dma_log(const char *hypothesis_id, const char *location, const char *stage, int used_qw, int dma_qwc) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"vif1_dma\",\"data\":{\"stage\":\"%s\",\"used_qw\":%d,\"dma_qwc\":%d},\"timestamp\":0}\n",
        hypothesis_id, location, stage, used_qw, dma_qwc
    );
    pantheon_log_flush_maybe(f);
}

/* H2/H3/H4: Track VU memory layout and shader upload metadata per batch. */
static void agent_path1_layout_log(
    const char *hypothesis_id,
    const char *location,
    int shader_qwc,
    int shader_instr,
    int verts_this_batch,
    int input_start,
    int input_end,
    int output_start,
    int output_end,
    int wraps,
    int overlaps
) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"vu1_layout\",\"data\":{\"shader_qwc\":%d,\"shader_instr\":%d,\"verts\":%d,\"in\":[%d,%d],\"out\":[%d,%d],\"wrap\":%d,\"overlap\":%d},\"timestamp\":0}\n",
        hypothesis_id,
        location,
        shader_qwc,
        shader_instr,
        verts_this_batch,
        input_start,
        input_end,
        output_start,
        output_end,
        wraps,
        overlaps
    );
    pantheon_log_flush_maybe(f);
}

/* H5/H6: Sample EE-side clip-space W and NDC ranges before VIF upload. */
static void agent_path1_clip_log(
    const char *hypothesis_id,
    const char *location,
    int verts_sampled,
    int w_le_0_1_count,
    float w_min,
    float w_max,
    float ndc_x_min,
    float ndc_x_max,
    float ndc_y_min,
    float ndc_y_max
) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"clip_sample\",\"data\":{\"verts\":%d,\"w_le_0_1\":%d,\"w_min\":%.5f,\"w_max\":%.5f,\"ndc_x\":[%.5f,%.5f],\"ndc_y\":[%.5f,%.5f]},\"timestamp\":0}\n",
        hypothesis_id,
        location,
        verts_sampled,
        w_le_0_1_count,
        w_min,
        w_max,
        ndc_x_min,
        ndc_x_max,
        ndc_y_min,
        ndc_y_max
    );
    pantheon_log_flush_maybe(f);
}

/* H3/H7: Record VU/GIF contract resolved by EE for first batch. */
static void agent_path1_contract_log(
    const char *hypothesis_id,
    const char *location,
    int gif_addr,
    int out_base,
    int xgkick_addr,
    int nloop
) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"vu_gif_contract\",\"data\":{\"gif_addr\":%d,\"out_base\":%d,\"xgkick_addr\":%d,\"nloop\":%d},\"timestamp\":0}\n",
        hypothesis_id, location, gif_addr, out_base, xgkick_addr, nloop
    );
    pantheon_log_flush_maybe(f);
}

/* H7: Correlate phase ordering with alternating clear shades. */
static void agent_path1_phase_log(const char *hypothesis_id, const char *location, const char *phase, int frame_id) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"frame_phase\",\"data\":{\"phase\":\"%s\",\"frame\":%d},\"timestamp\":0}\n",
        hypothesis_id, location, phase, frame_id
    );
    pantheon_log_flush_maybe(f);
}

/* H12: Validate emitted memory-image payload controls (PRIM/TME, ITOP, near-Z). */
static void agent_path1_payload_log(
    const char *hypothesis_id,
    const char *location,
    int nloop,
    int prim_word,
    int itop,
    float nearz
) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"path1_payload\",\"data\":{\"nloop\":%d,\"prim\":%d,\"itop\":%d,\"nearz\":%.4f},\"timestamp\":0}\n",
        hypothesis_id, location, nloop, prim_word, itop, nearz
    );
    pantheon_log_flush_maybe(f);
}

/* H13: Track packet pointer integrity and qword usage vs capacity. */
static void agent_path1_packet_log(
    const char *hypothesis_id,
    const char *location,
    const char *stage,
    int ctx,
    int cap_qw,
    int used_qw,
    int pkt_null
) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"packet_state\",\"data\":{\"stage\":\"%s\",\"ctx\":%d,\"cap_qw\":%d,\"used_qw\":%d,\"pkt_null\":%d},\"timestamp\":0}\n",
        hypothesis_id, location, stage, ctx, cap_qw, used_qw, pkt_null
    );
    pantheon_log_flush_maybe(f);
}

/* H16/H17/H18: Camera and VIF1 chain submit diagnostics. */
static void agent_path1_camera_log(
    const char *hypothesis_id,
    const char *location,
    float cam_x,
    float cam_y,
    float cam_z,
    float pitch,
    float yaw
) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"camera_state\",\"data\":{\"cam\":[%.3f,%.3f,%.3f],\"pitch\":%.4f,\"yaw\":%.4f},\"timestamp\":0}\n",
        hypothesis_id, location, cam_x, cam_y, cam_z, pitch, yaw
    );
    pantheon_log_flush_maybe(f);
}

/* H19: Guaranteed early boot marker before init steps. */
static void agent_path1_boot_log(const char *hypothesis_id, const char *location, const char *phase) {
    FILE *f = pantheon_log_stream_open(
        "host:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host0:/home/marvin/Pantheon/.cursor/debug-f266a2.log",
        "host:.cursor/debug-f266a2.log",
        "host:debug-f266a2.log");
    if (!f)
        return;
    fprintf(
        f,
        "{\"sessionId\":\"f266a2\",\"runId\":\"path1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"boot_phase\",\"data\":{\"phase\":\"%s\"},\"timestamp\":0}\n",
        hypothesis_id, location, phase
    );
    pantheon_log_flush_maybe(f);
}

#else
#define agent_path1_dma_log(hyp, loc, stage, used_qw, dma_qwc) ((void)0)
#define agent_path1_layout_log(hyp, loc, shader_qwc, shader_instr, verts_this_batch, input_start, input_end, output_start, output_end, wraps, overlaps) ((void)0)
#define agent_path1_clip_log(hypothesis_id, location, verts_sampled, w_le_0_1_count, w_min, w_max, ndc_x_min, ndc_x_max, ndc_y_min, ndc_y_max) ((void)0)
#define agent_path1_contract_log(hypothesis_id, location, gif_addr, out_base, xgkick_addr, nloop) ((void)0)
#define agent_path1_phase_log(hypothesis_id, location, phase, frame_id) ((void)0)
#define agent_path1_payload_log(hypothesis_id, location, nloop, prim_word, itop, nearz) ((void)0)
#define agent_path1_packet_log(hypothesis_id, location, stage, ctx, cap_qw, used_qw, pkt_null) ((void)0)
#define agent_path1_camera_log(hypothesis_id, location, cam_x, cam_y, cam_z, pitch, yaw) ((void)0)
#define agent_path1_boot_log(hypothesis_id, location, phase) ((void)0)
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
    static int lx_confirm = 0;
    static int ly_confirm = 0;
    static const int pad_left_stick_confirm_frames = 2;
    if (!pad_ready) {
        static int not_ready_samples = 0;
        not_ready_samples++;
        if (not_ready_samples <= 3 || (not_ready_samples % 120) == 0) {
            char j[128];
            snprintf(j, sizeof(j), "{\"pad_ready\":%d,\"samples\":%d}", pad_ready, not_ready_samples);
            // #region agent log
            agent_dbg22_log("pre-fix", "H21", "floor.c:read_pad_analog", "pad_not_ready_early_return", j);
            // #endregion
        }
        return;
    }

    struct padButtonStatus buttons;
    
    // Pre-center sticks in case the read fails or analog isn't fully on yet
    buttons.ljoy_h = 128;
    buttons.ljoy_v = 128;
    buttons.rjoy_h = 128;
    buttons.rjoy_v = 128;

    int read_ok = padRead(0, 0, &buttons);
    if (read_ok == 0) {
        static int read_fail_samples = 0;
        read_fail_samples++;
        if (read_fail_samples <= 3 || (read_fail_samples % 120) == 0) {
            char j[128];
            snprintf(j, sizeof(j), "{\"pad_ready\":%d,\"read_ok\":%d,\"samples\":%d}", pad_ready, read_ok, read_fail_samples);
            // #region agent log
            agent_dbg22_log("pre-fix", "H22", "floor.c:read_pad_analog", "pad_read_failed", j);
            // #endregion
        }
        return;
    }

    float lx_raw = (float)buttons.ljoy_h - 127.5f;
    float ly_raw = (float)buttons.ljoy_v - 127.5f;
    float lx = lx_raw;
    float ly = ly_raw;
    float rx_raw = (float)buttons.rjoy_h - 127.5f;
    float ry_raw = (float)buttons.rjoy_v - 127.5f;
    float rx = rx_raw;
    float ry = ry_raw;

    if (fabsf(lx) < PAD_DEADZONE) {
        lx = 0.0f;
        lx_confirm = 0;
    } else {
        lx_confirm++;
        if (lx_confirm < pad_left_stick_confirm_frames) lx = 0.0f;
    }
    if (fabsf(ly) < PAD_DEADZONE) {
        ly = 0.0f;
        ly_confirm = 0;
    } else {
        ly_confirm++;
        if (ly_confirm < pad_left_stick_confirm_frames) ly = 0.0f;
    }
    if (fabsf(rx) < PAD_DEADZONE) rx = 0.0f;
    if (fabsf(ry) < PAD_DEADZONE) ry = 0.0f;
#if PANTHEON_TRIAGE_LOCK_RIGHT_STICK
    rx = 0.0f;
    ry = 0.0f;
#endif
    if (g_dbg_frame <= 3 || (g_dbg_frame % 120) == 0) {
        char j[256];
        snprintf(
            j,
            sizeof(j),
            "{\"frame\":%d,\"rjoy_raw\":[%.2f,%.2f],\"rjoy_dz\":[%.2f,%.2f],\"target\":[%.4f,%.4f],\"cam\":[%.4f,%.4f]}",
            g_dbg_frame,
            rx_raw, ry_raw,
            rx, ry,
            target_yaw, target_pitch,
            cam_yaw, cam_pitch
        );
        // #region agent log
        dbg22_log_json("triage-run", "H7", "floor.c:read_pad_analog", "right_stick_drift", j);
        // #endregion
    }

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

    if (g_dbg_frame <= 3 || (g_dbg_frame % 120) == 0) {
        char j[320];
        snprintf(
            j,
            sizeof(j),
            "{\"frame\":%d,\"ljoy_raw\":[%.2f,%.2f],\"ljoy_dz\":[%.2f,%.2f],\"dpad\":[%d,%d],\"mv_pre\":[%.2f,%.2f],\"player_xz\":[%.3f,%.3f]}",
            g_dbg_frame,
            lx_raw, ly_raw,
            lx, ly,
            dpx, dpy,
            mvx, mvy,
            player_x, player_z
        );
        // #region agent log
        dbg22_log_json("triage-run", "H35", "floor.c:read_pad_analog", "movement_intent", j);
        // #endregion
    }

    /* "Up" pulls the ground toward the player (negate Y). Invert left/right on stick + D-pad (do not negate X). */
    mvy = -mvy;
    float mvx_post = mvx;
    float mvy_post = mvy;

    target_yaw   += rx * RIGHT_YAW_SENS;
    target_pitch += ry * RIGHT_PITCH_SENS;
    target_yaw = wrap_angle_pi(target_yaw);

#if PANTHEON_CAMERA_ENVELOPE_HARDEN
    if (target_pitch < 0.20f) target_pitch = 0.20f;
    if (target_pitch > 1.15f) target_pitch = 1.15f;
#else
    if (target_pitch < 0.08f)  target_pitch = 0.08f;
    if (target_pitch > 1.32f)  target_pitch = 1.32f;
#endif

    cam_yaw   += (target_yaw   - cam_yaw)   * 0.165f;
    cam_pitch += (target_pitch - cam_pitch) * 0.155f;
    cam_yaw = wrap_angle_pi(cam_yaw);
#if PANTHEON_CAMERA_ENVELOPE_HARDEN
    if (cam_pitch < 0.20f) cam_pitch = 0.20f;
    if (cam_pitch > 1.15f) cam_pitch = 1.15f;
#endif

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
        if (g_dbg_frame <= 3 || (g_dbg_frame % 120) == 0) {
            char j[320];
            snprintf(
                j,
                sizeof(j),
                "{\"frame\":%d,\"mv\":[%.2f,%.2f],\"mv_post\":[%.2f,%.2f],\"mag\":%.3f,\"move_scale\":%.3f,\"move_xz\":[%.3f,%.3f],\"player_xz\":[%.3f,%.3f],\"cam_yaw\":%.4f}",
                g_dbg_frame,
                mvx, mvy,
                mvx_post, mvy_post,
                mag,
                move_scale,
                move_x, move_z,
                player_x, player_z,
                cam_yaw
            );
            // #region agent log
            dbg22_log_json("triage-run", "H31", "floor.c:read_pad_analog", "player_motion_step", j);
            // #endregion
        }
    }

    clamp_player_to_floor();
}

static void update_camera_orbit(void) {
#if PANTHEON_FIRST_PERSON_VIEW
    camera_position[0] = player_x;
    camera_position[1] = player_y + PANTHEON_FIRST_PERSON_EYE_HEIGHT;
    camera_position[2] = player_z;
    camera_position[3] = 1.0f;

    camera_rotation[0] = PANTHEON_TRIAGE_PITCH_SIGN_FLIP ? cam_pitch : -cam_pitch;
    camera_rotation[1] = -cam_yaw;
    camera_rotation[2] = 0.0f;
    camera_rotation[3] = 1.0f;
#else
    float cos_p = cosf(cam_pitch);
    float sin_p = sinf(cam_pitch);

    camera_position[0] = player_x + (sinf(cam_yaw) * cos_p * CAM_DIST);
    camera_position[1] = player_y + (sin_p * CAM_DIST) + PANTHEON_CAMERA_HEIGHT_OFFSET;
    camera_position[2] = player_z + (cosf(cam_yaw) * cos_p * CAM_DIST);
    camera_position[3] = 1.0f;

    camera_rotation[0] = PANTHEON_TRIAGE_PITCH_SIGN_FLIP ? cam_pitch : -cam_pitch;
    camera_rotation[1] = -cam_yaw;
    camera_rotation[2] = 0.0f;
    camera_rotation[3] = 1.0f;
#endif
    if (g_dbg_frame <= 3 || (g_dbg_frame % 120) == 0) {
        float dx = camera_position[0] - player_x;
        float dz = camera_position[2] - player_z;
        float planar = sqrtf((dx * dx) + (dz * dz));
        float floor_y_mid = 0.0f;
        char j[320];
        snprintf(
            j,
            sizeof(j),
            "{\"frame\":%d,\"player\":[%.3f,%.3f,%.3f],\"cam\":[%.3f,%.3f,%.3f],\"offset_planar\":%.3f,\"cam_dist\":%.3f,\"pitch\":%.4f,\"floor_y_mid\":%.3f,\"cam_minus_floor\":%.3f,\"player_minus_floor\":%.3f}",
            g_dbg_frame,
            player_x, player_y, player_z,
            camera_position[0], camera_position[1], camera_position[2],
            planar,
            CAM_DIST,
            cam_pitch,
            floor_y_mid,
            camera_position[1] - floor_y_mid,
            player_y - floor_y_mid
        );
        // #region agent log
        dbg22_log_json("triage-run", "H32", "floor.c:update_camera_orbit", "camera_follow_offset", j);
        // #endregion
    }
}

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
        g_atmosphere = pantheon_lerp_atmosphere(a_from, a_to, bt);
        g_weather_blend_remaining--;
        if (g_weather_blend_remaining == 0) {
            g_weather = w_target;
        }
    } else {
        g_weather = w_target;
        g_atmosphere = pantheon_sample_timecycle(g_weather, g_day01);
    }

    update_skydome_colors();
}

/* Flattened triangle stream for VU1 (CPU expand at boot; see flight log Day 4). */
PantheonVertex flat_floor[FLOOR_TRI_COUNT * 3] __attribute__((aligned(16)));
static PantheonVertex floor_tile_tris[FLOOR_TRI_COUNT * 3] __attribute__((aligned(16)));
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
    // #region agent log
    {
        char j[224];
        snprintf(
            j,
            sizeof(j),
            "{\"frame\":%d,\"floor_bounds\":{\"x\":[%.3f,%.3f],\"y\":[%.3f,%.3f],\"z\":[%.3f,%.3f]}}",
            g_dbg_frame,
            floor_bound_x0, floor_bound_x1,
            floor_bound_y0, floor_bound_y1,
            floor_bound_z0, floor_bound_z1
        );
        dbg22_log_json("triage-height", "H40", "floor.c:init_floor_walk_bounds", "floor_mesh_bounds", j);
    }
    // #endregion
}

static void clamp_player_to_floor(void) {
#if PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER
    if (g_dbg_frame <= 3 || (g_dbg_frame % 240) == 0) {
        char j[160];
        snprintf(
            j,
            sizeof(j),
            "{\"frame\":%d,\"mode\":\"triage_follow_player\",\"clamp_enabled\":0}",
            g_dbg_frame
        );
        // #region agent log
        dbg22_log_json("triage-run", "H34", "floor.c:clamp_player_to_floor", "player_clamp_bounds", j);
        // #endregion
    }
    return;
#endif
    /* SoftImage sandbox floor contract: player always stands on floor plane (y=0). */
    player_y = 0.0f;

    /* Keep world position in a huge envelope to avoid far-distance precision drift. */
    float limit_x = floor_tile_span_x * 100.0f;
    float limit_z = floor_tile_span_z * 100.0f;
    float before_x = player_x;
    float before_z = player_z;
    int clamped_x = 0;
    int clamped_z = 0;
    if (floor_tile_span_x > 0.0f) {
        if (player_x > limit_x) {
            player_x = limit_x;
            clamped_x = 1;
        }
        if (player_x < -limit_x) {
            player_x = -limit_x;
            clamped_x = 1;
        }
    }
    if (floor_tile_span_z > 0.0f) {
        if (player_z > limit_z) {
            player_z = limit_z;
            clamped_z = 1;
        }
        if (player_z < -limit_z) {
            player_z = -limit_z;
            clamped_z = 1;
        }
    }
    if (clamped_x || clamped_z || g_dbg_frame <= 3 || (g_dbg_frame % 240) == 0) {
        char j[288];
        snprintf(
            j,
            sizeof(j),
            "{\"frame\":%d,\"before\":[%.3f,%.3f],\"after\":[%.3f,%.3f],\"clamped\":[%d,%d],\"limit\":[%.3f,%.3f],\"span\":[%.3f,%.3f]}",
            g_dbg_frame,
            before_x, before_z,
            player_x, player_z,
            clamped_x, clamped_z,
            limit_x, limit_z,
            floor_tile_span_x, floor_tile_span_z
        );
        // #region agent log
        dbg22_log_json("triage-run", "H34", "floor.c:clamp_player_to_floor", "player_clamp_bounds", j);
        // #endregion
    }
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
    batch->constants[0] = 320.0f;
    batch->constants[1] = 224.0f;
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
    int itops_val = batch_qwc;
    if (g_dbg_frame <= 3 || (g_dbg_frame % 120) == 0) {
        // #region agent log
        agent_path1_payload_log("H12", "floor.c:path1_emit_memory_image_batch", batch->vert_count, GIF_PRIM_TRIANGLE | 8, itops_val, batch->constants[6]);
        {
            char j[192];
            snprintf(
                j,
                sizeof(j),
                "{\"frame\":%d,\"verts\":%d,\"in_qw_per_vert\":%d,\"batch_qwc\":%d,\"itop\":%d}",
                g_dbg_frame,
                batch->vert_count,
                PATH1_VU_QW_PER_VERT,
                batch_qwc,
                itops_val);
            dbg22_log_json("triage-run", "H36", "floor.c:path1_emit_memory_image_batch", "vif_itop_stride", j);
        }
        // #endregion
    }

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
    int vif1_qw = (int)(q_end - pkt->data);
    g_last_vif1_qw = vif1_qw;
    if (frame_id <= 3 || (frame_id % 120) == 0) {
        agent_path1_dma_log("H17", "floor.c:path1_vif_packet_submit", tag, vif1_qw, vif1_qw);
    }
    dma_channel_send_chain(DMA_CHANNEL_VIF1, (void *)PHYSICAL(pkt->data), vif1_qw, 0, 0);
    dma_wait_fast();
    g_vif_context ^= 1;
}

// Unroll the indices into a flat triangle array at boot
void init_flat_floor() {
    int out_idx = 0;
    for (int i = 0; i < FLOOR_TRI_COUNT; i++) {
#if PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
        /* SoftImage floor grid is authored in the mesh XY plane (Z≈0). Map to world XZ with
         * Y up. (a,c,b) winding makes normals face +Y after XY→XZ (single-sided Path1). */
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
    /* Grass-ish base (Bliss-style); tune packed bytes if hue shifts on hardware. */
    color_pun.i = (128 << 24) | (55 << 16) | (185 << 8) | 40;

    for (int i = 0; i < FLOOR_TRI_COUNT * 3; i++) {
        float src_x = flat_floor[i].x;
        float src_y = flat_floor[i].y;
#if PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE
        flat_floor[i].x = src_x * FLOOR_MESH_SCALE;
        flat_floor[i].y = 0.0f;
        flat_floor[i].z = src_y * FLOOR_MESH_SCALE;
#else
        float src_z = flat_floor[i].z;
        /* Mesh already in XZ footprint; scale both axes. */
        flat_floor[i].x = src_x * FLOOR_MESH_SCALE;
        flat_floor[i].y = 0.0f;
        flat_floor[i].z = src_z * FLOOR_MESH_SCALE;
#endif

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
}

static void build_floor_tile(float tile_off_x, float tile_off_z) {
    for (int i = 0; i < FLOOR_TRI_COUNT * 3; i++) {
        floor_tile_tris[i] = flat_floor[i];
        floor_tile_tris[i].x += tile_off_x;
        floor_tile_tris[i].z += tile_off_z;
    }
}

#if PANTHEON_TRIAGE_FORCE_FLAT_QUAD
static void build_floor_follow_quad(float center_x, float center_z) {
    float half_x = (floor_tile_span_x > 0.0f) ? (floor_tile_span_x * 0.5f) : 420.0f;
    float half_z = (floor_tile_span_z > 0.0f) ? (floor_tile_span_z * 0.5f) : 420.0f;
    PantheonColorPun color_pun;
    color_pun.i = (128 << 24) | (55 << 16) | (185 << 8) | 40;

    float x0 = center_x - half_x;
    float x1 = center_x + half_x;
    float z0 = center_z - half_z;
    float z1 = center_z + half_z;
    float y = 0.0f;

    PantheonVertex v0 = {x0, y, z0, color_pun.f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    PantheonVertex v1 = {x1, y, z0, color_pun.f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
    PantheonVertex v2 = {x0, y, z1, color_pun.f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f};
    PantheonVertex v3 = {x1, y, z1, color_pun.f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f};

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

        int input_start = PATH1_VU_INPUT_BASE;
        int input_end = input_start + (verts_this_batch * 3) - 1;
        int output_start = PATH1_VU_OUTPUT_BASE;
        int output_end = output_start + (verts_this_batch * 3) - 1;
        int wraps = (output_end > 1023) ? 1 : 0;
        int overlaps = ((output_start <= input_end) && (output_end >= input_start)) ? 1 : 0;
#if !PANTHEON_AGENT_TRACE
        (void)wraps;
        (void)overlaps;
#endif

        if (i == 0 && (g_dbg_frame <= 3 || (g_dbg_frame % 120) == 0)) {
            int sample_count = (verts_this_batch < 8) ? verts_this_batch : 8;
            int w_le_0_1_count = 0;
            int ndc_oob_count = 0;
            float w_min = 1.0e30f, w_max = -1.0e30f;
            float ndc_x_min = 1.0e30f, ndc_x_max = -1.0e30f;
            float ndc_y_min = 1.0e30f, ndc_y_max = -1.0e30f;
            for (int sv = 0; sv < sample_count; sv++) {
                const PantheonVertex *v = &mesh_tris[i + sv];
                float cx = (mvp[0] * v->x) + (mvp[4] * v->y) + (mvp[8] * v->z) + (mvp[12] * v->w);
                float cy = (mvp[1] * v->x) + (mvp[5] * v->y) + (mvp[9] * v->z) + (mvp[13] * v->w);
                float cw = (mvp[3] * v->x) + (mvp[7] * v->y) + (mvp[11] * v->z) + (mvp[15] * v->w);
                if (cw <= PANTHEON_VU_NEARZ) w_le_0_1_count++;
                if (cw < w_min) w_min = cw;
                if (cw > w_max) w_max = cw;
                if (fabsf(cw) > 1.0e-6f) {
                    float ndc_x = cx / cw;
                    float ndc_y = cy / cw;
                    if (ndc_x < ndc_x_min) ndc_x_min = ndc_x;
                    if (ndc_x > ndc_x_max) ndc_x_max = ndc_x;
                    if (ndc_y < ndc_y_min) ndc_y_min = ndc_y;
                    if (ndc_y > ndc_y_max) ndc_y_max = ndc_y;
                    if (fabsf(ndc_x) > 1.0f || fabsf(ndc_y) > 1.0f) {
                        ndc_oob_count++;
                    }
                }
            }
            // #region agent log
            agent_path1_layout_log(
                "H2",
                "floor.c:render_path1_tris",
                (int)shader_qwc,
                (int)shader_instr,
                verts_this_batch,
                input_start,
                input_end,
                output_start,
                output_end,
                wraps,
                overlaps
            );
            agent_path1_contract_log(
                "H3",
                "floor.c:render_path1_tris",
                PATH1_VU_GIFTAG_ADDR,
                PATH1_VU_OUTPUT_BASE,
                PATH1_VU_XGKICK_ADDR,
                verts_this_batch
            );
            agent_path1_clip_log(
                "H5",
                "floor.c:render_path1_tris",
                sample_count,
                w_le_0_1_count,
                w_min,
                w_max,
                ndc_x_min,
                ndc_x_max,
                ndc_y_min,
                ndc_y_max
            );
            {
                char j[256];
                snprintf(
                    j,
                    sizeof(j),
                    "{\"frame\":%d,\"mesh\":\"%s\",\"sample_count\":%d,\"w_le_nearz\":%d,\"ndc_oob\":%d,\"ndc_x\":[%.3f,%.3f],\"ndc_y\":[%.3f,%.3f],\"w_min\":%.5f,\"w_max\":%.5f,\"nearz\":%.3f}",
                    g_dbg_frame,
                    g_path1_mesh_stage,
                    sample_count,
                    w_le_0_1_count,
                    ndc_oob_count,
                    ndc_x_min,
                    ndc_x_max,
                    ndc_y_min,
                    ndc_y_max,
                    w_min,
                    w_max,
                    PANTHEON_VU_NEARZ
                );
                // #region agent log
                dbg22_log_json("triage-run", "H1", "floor.c:render_path1_tris", "clipspace_w_sample", j);
                // #endregion
            }
            // #endregion
            batch_nearz_hits = w_le_0_1_count;
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
    return render_path1_tris(q, mvp, flat_floor, total_verts, FLOOR_TRI_COUNT * 3);
}

qword_t *render_clear_and_setup(qword_t *q, int ctx, framebuffer_t *frame, zbuffer_t *z) {
    q = draw_setup_environment(q, 0, &frame[ctx], z);
    q = draw_primitive_xyoffset(q, 0, 2048 - 320, 2048 - 224);
    /* Clear to active horizon RGB (must match skydome rim + apply_sky_color source colors). */
    q = draw_clear(
        q,
        0,
        2048.0f - 320.0f,
        2048.0f - 224.0f,
        640.0f,
        448.0f,
        g_atmosphere.sky_horizon.r,
        g_atmosphere.sky_horizon.g,
        g_atmosphere.sky_horizon.b);
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
    // #region agent log
    agent_path1_boot_log("H19", "floor.c:main", "entered_main_after_sifinit");
    // #endregion

    int sio2_res = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    int padman_res = SifLoadModule("rom0:PADMAN", 0, NULL);
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", (sio2_res >= 0 && padman_res >= 0) ? "pad_modules_loaded" : "pad_modules_failed");
    // #endregion
    if (sio2_res < 0 || padman_res < 0) {
        return 1;
    }

    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "before_pad_init");
    // #endregion
    padInit(0);
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "after_pad_init");
    // #endregion
    int pad_open_res = padPortOpen(0, 0, padBuf);
    pad_port_open_ok = (pad_open_res != 0);
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "after_pad_port_open");
    {
        char j[160];
        snprintf(j, sizeof(j), "{\"sio2\":%d,\"padman\":%d,\"pad_open\":%d,\"pad_ready\":%d}", sio2_res, padman_res, pad_open_res, pad_ready);
        agent_dbg22_log("pre-fix", "H23", "floor.c:main", "pad_boot_state", j);
    }
    // #endregion

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
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "after_graph_initialize");
    // #endregion
    graph_set_framebuffer(0, frame[0].address, frame[0].width, frame[0].psm, 0, 0);
    graph_enable_output();
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "after_graph_enable_output");
    // #endregion

    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
    /* Intermittent Mode Transfer: Path 3 texture uploads yield to Path 1 every 8 QW (verify vs SCE). */
    GIF_REG_MODE |= (1u << 2);
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "after_dma_init");
    // #endregion

    packets[0] = packet_init(4000, PACKET_NORMAL);
    packets[1] = packet_init(4000, PACKET_NORMAL);
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "after_packet_init");
    // #endregion

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
    /* Sample atmosphere before skydome vertex colors so apply_sky_color sees valid g_atmosphere. */
    g_atmosphere = pantheon_sample_timecycle(g_weather, g_day01);
#if USE_VU1_SKYDOME_MESH
    init_flat_skydome();
#endif
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "after_asset_init");
    // #endregion

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
    printf("pantheon strict_profile=%d overlay=%d tile_radius=%d tile_budget=%d\n",
           PANTHEON_RENDER_PROFILE, PATH1_AB_CPU_OVERLAY, PANTHEON_TILE_RADIUS, PANTHEON_TILE_SUBMIT_BUDGET);
    printf("pantheon triage static_cam=%d sky=%d view_near=%.2f vu_nearz=%.2f envelope=%d\n",
           PANTHEON_TRIAGE_STATIC_CAMERA,
           PANTHEON_TRIAGE_ENABLE_SKYDOME,
           PANTHEON_VIEW_NEAR,
           PANTHEON_VU_NEARZ,
           PANTHEON_CAMERA_ENVELOPE_HARDEN);
    printf("pantheon skydome mesh path=%d tris=%d\n", USE_VU1_SKYDOME_MESH, SKYDOME_TRI_COUNT);
    {
        char j[320];
        snprintf(
            j,
            sizeof(j),
            "{\"static_cam\":%d,\"sky\":%d,\"view_near\":%.3f,\"vu_nearz\":%.3f,\"envelope\":%d,\"yaw_guard\":%d,\"pitch_flip\":%d,\"frustum_lr\":%.1f,\"frustum_bt\":%.1f}",
            PANTHEON_TRIAGE_STATIC_CAMERA,
            PANTHEON_TRIAGE_ENABLE_SKYDOME,
            PANTHEON_VIEW_NEAR,
            PANTHEON_VU_NEARZ,
            PANTHEON_CAMERA_ENVELOPE_HARDEN,
            PANTHEON_TRIAGE_WORLDVIEW_YAW_GUARD,
            PANTHEON_TRIAGE_PITCH_SIGN_FLIP,
            PANTHEON_VIEW_FRUSTUM_LR,
            PANTHEON_VIEW_FRUSTUM_BT
        );
        // #region agent log
        dbg22_log_json("triage-run", "H4", "floor.c:main", "triage_config", j);
        // #endregion
    }
    // #region agent log
    agent_path1_boot_log("H20", "floor.c:main", "before_main_loop");
    // #endregion

    while (1) {
        static int dbg_frame = 0;
        dbg_frame++;
        g_dbg_frame = dbg_frame;
        update_atmosphere(dbg_frame);
        packet_reset(packets[context]);
        qword_t *q = packets[context]->data;
        const PantheonRenderJob render_jobs[] = {
            {PANTHEON_RENDER_JOB_CLEAR, 1},
            {PANTHEON_RENDER_JOB_CPU_DEBUG, PATH1_AB_CPU_OVERLAY},
            {PANTHEON_RENDER_JOB_PATH1_SKY, (RENDER_STAGE >= 2) && PANTHEON_TRIAGE_ENABLE_SKYDOME},
            {PANTHEON_RENDER_JOB_PATH1_FLOOR, RENDER_STAGE >= 2}
        };

        if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
            int pad_state = padGetState(0, 0);
            char j[256];
            snprintf(
                j,
                sizeof(j),
                "{\"frame\":%d,\"pad_ready\":%d,\"pad_state\":%d,\"render_stage\":%d,\"cam\":[%.3f,%.3f,%.3f],\"rot\":[%.4f,%.4f]}",
                dbg_frame,
                pad_ready,
                pad_state,
                RENDER_STAGE,
                camera_position[0], camera_position[1], camera_position[2],
                camera_rotation[0], camera_rotation[1]
            );
            // #region agent log
            agent_dbg22_log("pre-fix", "H24", "floor.c:main_loop", "frame_input_camera_state", j);
            dbg22_log_json("triage-run", "H2", "floor.c:main_loop", "camera_pose_sample", j);
            // #endregion
        }
#if PANTHEON_MIN_TELEMETRY
        if ((dbg_frame % 300) == 0) {
            printf("pantheon frame=%d overlay=%d cam=(%.2f,%.2f,%.2f)\n",
                   dbg_frame,
                   PATH1_AB_CPU_OVERLAY,
                   camera_position[0], camera_position[1], camera_position[2]);
        }
#endif

        try_finish_pad_init();
#if !PANTHEON_TRIAGE_STATIC_CAMERA
        read_pad_analog();
        update_camera_orbit();
#else
        /* Center sandbox camera over the authored 6x6 floor grid. */
        camera_position[0] = g_floor_center_x;
        camera_position[1] = 260.0f;
        camera_position[2] = g_floor_center_z;
        camera_position[3] = 1.0f;
        camera_rotation[0] = PANTHEON_TRIAGE_PITCH_SIGN_FLIP ? 1.15f : -1.15f;
        camera_rotation[1] = 0.0f;
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
        if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
            float cx = (mvp[0] * g_floor_center_x) + (mvp[4] * 0.0f) + (mvp[8] * g_floor_center_z) + mvp[12];
            float cy = (mvp[1] * g_floor_center_x) + (mvp[5] * 0.0f) + (mvp[9] * g_floor_center_z) + mvp[13];
            float cw = (mvp[3] * g_floor_center_x) + (mvp[7] * 0.0f) + (mvp[11] * g_floor_center_z) + mvp[15];
            float ndc_x = (fabsf(cw) > 1.0e-6f) ? (cx / cw) : 9999.0f;
            float ndc_y = (fabsf(cw) > 1.0e-6f) ? (cy / cw) : 9999.0f;
            char j[256];
            snprintf(
                j,
                sizeof(j),
                "{\"frame\":%d,\"floor_center\":[%.2f,%.2f],\"cw\":%.4f,\"ndc\":[%.3f,%.3f],\"cam\":[%.2f,%.2f,%.2f]}",
                dbg_frame,
                g_floor_center_x, g_floor_center_z,
                cw, ndc_x, ndc_y,
                camera_position[0], camera_position[1], camera_position[2]
            );
            // #region agent log
            dbg22_log_json("triage-run", "H6", "floor.c:main_loop", "floor_center_view", j);
            {
                char j2[224];
                snprintf(
                    j2,
                    sizeof(j2),
                    "{\"frame\":%d,\"cam_y\":%.3f,\"player_y\":%.3f,\"cam_minus_player_y\":%.3f,\"cam_dist\":%.3f,\"pitch\":%.4f,\"yaw_guard\":%d,\"view_lrbt\":[-%.1f,%.1f,-%.1f,%.1f],\"near_far\":[%.3f,%.1f]}",
                    dbg_frame,
                    camera_position[1],
                    player_y,
                    camera_position[1] - player_y,
                    CAM_DIST,
                    cam_pitch,
                    PANTHEON_TRIAGE_WORLDVIEW_YAW_GUARD,
                    PANTHEON_VIEW_FRUSTUM_LR,
                    PANTHEON_VIEW_FRUSTUM_LR,
                    PANTHEON_VIEW_FRUSTUM_BT,
                    PANTHEON_VIEW_FRUSTUM_BT,
                    PANTHEON_VIEW_NEAR,
                    g_atmosphere.far_clip
                );
                dbg22_log_json("triage-height", "H42", "floor.c:main_loop", "camera_view_envelope", j2);
            }
            // #endregion
        }
        if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
            // #region agent log
            agent_path1_phase_log("H16", "floor.c:main_loop", "frame_enter", dbg_frame);
            agent_path1_camera_log("H18", "floor.c:main_loop", camera_position[0], camera_position[1], camera_position[2], camera_rotation[0], camera_rotation[1]);
            // #endregion
        }

        if (render_jobs[0].enabled) {
            q = render_clear_and_setup(q, context, frame, &z);
        }

        int cpu_qw_before = (int)(q - packets[context]->data);
        if (render_jobs[1].enabled) {
            q = render_cpu_probe(q, world_view, view_screen);
            q = render_cpu_exported_floor(q, world_view, view_screen);
        }
        if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
            int cpu_qw_after = (int)(q - packets[context]->data);
            char j[128];
            snprintf(j, sizeof(j), "{\"frame\":%d,\"cpu_qw_added\":%d}", dbg_frame, cpu_qw_after - cpu_qw_before);
            // #region agent log
            agent_dbg22_log("pre-fix", "H25", "floor.c:main_loop", "cpu_path_qw_usage", j);
            // #endregion
        }

        FlushCache(0);
        if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
            // #region agent log
            agent_path1_phase_log("H16", "floor.c:main_loop", "gif_send", dbg_frame);
            // #endregion
        }
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
                q = render_path1_tris(q, mvp, flat_skydome, SKYDOME_TRI_COUNT * 3, SKYDOME_TRI_COUNT * 3);
                q = add_dma_tag(q, 0, 7, 0, 0, 0);
                FlushCache(0);
                if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
                    int vif1_qw = (int)(q - vif_pkt->data);
                    agent_path1_phase_log("H7", "floor.c:main_loop", "skydome_send", dbg_frame);
                    agent_path1_packet_log("H13", "floor.c:main_loop", "skydome_send", g_vif_context, (int)vif_pkt->qwords, vif1_qw, 0);
                    (void)vif1_qw;
                }
                path1_vif_packet_submit(vif_pkt, q, dbg_frame, "skydome");
            }
        #endif
            if (render_jobs[3].enabled) {
                int path1_vert_count = FLOOR_TRI_COUNT * 3;
                int tile_submits = 0;
#if PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER
                {
                    packet_t *vif_pkt = path1_vif_packet_begin();
                    q = vif_pkt->data;
                    /* Triage: keep one floor tile centered on player to validate camera/placement without world paging noise. */
                    g_floor_center_x = player_x;
                    g_floor_center_z = player_z;
#if PANTHEON_TRIAGE_FORCE_FLAT_QUAD
                    build_floor_follow_quad(player_x, player_z);
                    path1_vert_count = 6;
#else
                    build_floor_tile(player_x, player_z);
#endif
                    if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
                        const PantheonVertex *sample_mesh = floor_tile_tris;
                        int tile_n = FLOOR_TRI_COUNT * 3;
#if PANTHEON_TRIAGE_FORCE_FLAT_QUAD
                        sample_mesh = floor_follow_quad;
                        tile_n = 6;
#endif
                        float tile_y_min = sample_mesh[0].y;
                        float tile_y_max = sample_mesh[0].y;
                        float ndc_x_min = 9999.0f;
                        float ndc_x_max = -9999.0f;
                        float ndc_y_min = 9999.0f;
                        float ndc_y_max = -9999.0f;
                        int ndc_valid = 0;
                        for (int vi = 1; vi < tile_n; vi++) {
                            float vy = sample_mesh[vi].y;
                            if (vy < tile_y_min) tile_y_min = vy;
                            if (vy > tile_y_max) tile_y_max = vy;
                            if ((vi % 32) == 0) {
                                float vx = sample_mesh[vi].x;
                                float vz = sample_mesh[vi].z;
                                float cx = (mvp[0] * vx) + (mvp[4] * vy) + (mvp[8] * vz) + mvp[12];
                                float cy = (mvp[1] * vx) + (mvp[5] * vy) + (mvp[9] * vz) + mvp[13];
                                float cw = (mvp[3] * vx) + (mvp[7] * vy) + (mvp[11] * vz) + mvp[15];
                                if (fabsf(cw) > 1.0e-6f) {
                                    float nx = cx / cw;
                                    float ny = cy / cw;
                                    if (nx < ndc_x_min) ndc_x_min = nx;
                                    if (nx > ndc_x_max) ndc_x_max = nx;
                                    if (ny < ndc_y_min) ndc_y_min = ny;
                                    if (ny > ndc_y_max) ndc_y_max = ny;
                                    ndc_valid++;
                                }
                            }
                        }
                        char j[352];
                        snprintf(
                            j,
                            sizeof(j),
                            "{\"frame\":%d,\"tile_center\":[%.3f,%.3f],\"player_xz\":[%.3f,%.3f],\"cam_xz\":[%.3f,%.3f],\"tile_y\":[%.3f,%.3f],\"player_y\":%.3f,\"cam_y\":%.3f,\"yaw_pitch\":[%.4f,%.4f]}",
                            dbg_frame,
                            g_floor_center_x, g_floor_center_z,
                            player_x, player_z,
                            camera_position[0], camera_position[2],
                            tile_y_min, tile_y_max,
                            player_y, camera_position[1],
                            cam_yaw, cam_pitch
                        );
                        // #region agent log
                        dbg22_log_json("triage-run", "H33", "floor.c:main_loop", "floor_submit_center", j);
                        dbg22_log_json("triage-height", "H41", "floor.c:main_loop", "floor_tile_height_sample", j);
                        if (ndc_valid > 0) {
                            char j_cov[192];
                            snprintf(
                                j_cov,
                                sizeof(j_cov),
                                "{\"frame\":%d,\"ndc_x\":[%.3f,%.3f],\"ndc_y\":[%.3f,%.3f],\"samples\":%d}",
                                dbg_frame,
                                ndc_x_min, ndc_x_max,
                                ndc_y_min, ndc_y_max,
                                ndc_valid
                            );
                            dbg22_log_json("triage-height", "H43", "floor.c:main_loop", "floor_ndc_coverage", j_cov);
                        }
                        // #endregion
                    }
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
                    q = render_floor_path1_count(q, mvp, path1_vert_count);
                    q = add_dma_tag(q, 0, 7, 0, 0, 0);
                    FlushCache(0);
                    if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
                        // #region agent log
                        agent_path1_phase_log("H17", "floor.c:main_loop", "vif1_wait_begin", dbg_frame);
                        // #endregion
                    }
                    path1_vif_packet_submit(vif_pkt, q, dbg_frame, "floor_single");
                    tile_submits = 1;
                    if (dbg_frame <= 3 || (dbg_frame % 120) == 0) {
                        // #region agent log
                        agent_path1_phase_log("H17", "floor.c:main_loop", "vif1_wait_end", dbg_frame);
                        // #endregion
                    }
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
                    {
                        char j[256];
                        snprintf(
                            j,
                            sizeof(j),
                            "{\"frame\":%d,\"vif_qw\":%d,\"nearz_hits\":%d,\"tiles\":%d,\"cam\":[%.2f,%.2f,%.2f],\"rot\":[%.3f,%.3f]}",
                            dbg_frame,
                            g_last_vif1_qw,
                            g_last_nearz_hits,
                            g_last_tile_submits,
                            camera_position[0], camera_position[1], camera_position[2],
                            camera_rotation[0], camera_rotation[1]
                        );
                        // #region agent log
                        dbg22_log_json("triage-run", "H3", "floor.c:main_loop", "path1_perf_sample", j);
                        // #endregion
                    }
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