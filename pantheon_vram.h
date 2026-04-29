#ifndef PANTHEON_VRAM_H
#define PANTHEON_VRAM_H

/*
 * GS local memory is addressed in 32-bit words (see ps2sdk graph_vram.h).
 * Frame/Z: GRAPH_ALIGN_PAGE (2048 words). Textures/CLUT: GRAPH_ALIGN_BLOCK (64 words).
 */

#include <tamtypes.h>

#define PANTHEON_VRAM_MAX_WORDS   1048576u
#define PANTHEON_VRAM_PAGE_WORDS  2048u
#define PANTHEON_VRAM_BLOCK_WORDS 64u

#if defined(__cplusplus)
extern "C" {
#endif

unsigned int pantheon_vram_surface_extent_words(int width, int height, int psm, int alignment);

void pantheon_vram_linear_reset(void);

/*
 * Bump-allocate from GS local memory base (word 0). align_words must be a power of two
 * (use PANTHEON_VRAM_PAGE_WORDS or PANTHEON_VRAM_BLOCK_WORDS). Returns 0 on failure.
 */
unsigned int pantheon_vram_linear_alloc_words(unsigned int extent_words, unsigned int align_words);

/* Half-open ranges [lo, hi) in word addresses */
int pantheon_vram_ranges_overlap(unsigned int a_lo, unsigned int a_hi, unsigned int b_lo, unsigned int b_hi);

/*
 * Print FBP0/FBP1/ZBP/TBP bases and extents; warn on overlap (telemetry gate).
 * Pass tex_base = 0 and tex_extent = 0 when no texture heap is allocated.
 */
void pantheon_vram_log_layout(
    unsigned int fbp0, unsigned int fbp0_extent,
    unsigned int fbp1, unsigned int fbp1_extent,
    unsigned int zbp, unsigned int zb_extent,
    unsigned int tbp, unsigned int tex_extent,
    int has_texture);

#if defined(__cplusplus)
}
#endif

#endif /* PANTHEON_VRAM_H */
