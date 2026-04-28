#ifndef PANTHEON_PATH1_CONTRACT_H
#define PANTHEON_PATH1_CONTRACT_H

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

#if (PATH1_VU_OUTPUT_BASE <= PATH1_VU_INPUT_BASE)
#error "Path 1 output buffer must live above the input payload."
#endif

#if ((PATH1_VU_OUTPUT_BASE + (PATH1_VU_MAX_BATCH_VERTS * 2)) > 1024)
#error "Path 1 output buffer exceeds VU1 data memory."
#endif

#endif /* PANTHEON_PATH1_CONTRACT_H */
