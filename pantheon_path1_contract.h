#ifndef PANTHEON_PATH1_CONTRACT_H
#define PANTHEON_PATH1_CONTRACT_H

#include <tamtypes.h>

/*
 * Shared EE/VU1 Path 1 memory contract.
 *
 * Keep these addresses synchronized with shader.vsm. The EE packet builder
 * unpacks constants and vertex payloads into these VU1 data-memory slots, and
 * shader.vsm xgkicks the GS packet starting at PATH1_VU_XGKICK_ADDR.
 */
#define PATH1_VU_GIFTAG_ADDR 240
#define PATH1_VU_OUTPUT_BASE 241
#define PATH1_VU_XGKICK_ADDR 240
#define PATH1_VU_INPUT_BASE 8
#define PATH1_VU_CONST_PERSPECTIVE_ADDR 0
#define PATH1_VU_CONST_SCREEN_ADDR 2
#define PATH1_VU_CONST_MVP_ADDR 4
#define PATH1_VU_MAX_BATCH_VERTS 108
#define PATH1_VU_MEM_QW 1024
#define PATH1_VU_QW_PER_VERT 2
#define PATH1_VU_BATCH_QW (PATH1_VU_MAX_BATCH_VERTS * PATH1_VU_QW_PER_VERT)

typedef union PantheonColorPun {
    float f;
    u32 i;
} PantheonColorPun;

#if (PATH1_VU_OUTPUT_BASE <= PATH1_VU_INPUT_BASE)
#error "Path 1 output buffer must live above the input payload."
#endif

#if ((PATH1_VU_OUTPUT_BASE + PATH1_VU_BATCH_QW) > PATH1_VU_MEM_QW)
#error "Path 1 output buffer exceeds VU1 data memory."
#endif

#if ((PATH1_VU_INPUT_BASE + PATH1_VU_BATCH_QW) > PATH1_VU_OUTPUT_BASE)
#error "Path 1 input batch overlaps VU output base."
#endif

#if ((PATH1_VU_CONST_MVP_ADDR + 4) > PATH1_VU_INPUT_BASE)
#error "Path 1 constants overlap input base."
#endif

#if (PATH1_VU_XGKICK_ADDR != PATH1_VU_GIFTAG_ADDR)
#error "Path 1 XGKICK address must match giftag address."
#endif

#endif /* PANTHEON_PATH1_CONTRACT_H */
