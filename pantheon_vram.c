#include "pantheon_vram.h"
#include <graph_vram.h>
#include <stdio.h>

static unsigned int g_vram_linear_cursor;

unsigned int pantheon_vram_surface_extent_words(int width, int height, int psm, int alignment)
{
    int sz = graph_vram_size(width, height, psm, alignment);
    if (sz < 0)
        return 0u;
    return (unsigned int)sz;
}

void pantheon_vram_linear_reset(void)
{
    g_vram_linear_cursor = 0u;
}

unsigned int pantheon_vram_linear_alloc_words(unsigned int extent_words, unsigned int align_words)
{
    if (extent_words == 0u || align_words == 0u)
        return 0u;
    if ((align_words & (align_words - 1u)) != 0u)
        return 0u;
    {
        unsigned int base = (g_vram_linear_cursor + align_words - 1u) & ~(align_words - 1u);
        unsigned int next = base + extent_words;
        if (next > PANTHEON_VRAM_MAX_WORDS || next < base)
            return 0u;
        g_vram_linear_cursor = next;
        return base;
    }
}

int pantheon_vram_ranges_overlap(unsigned int a_lo, unsigned int a_hi, unsigned int b_lo, unsigned int b_hi)
{
    if (a_lo >= a_hi || b_lo >= b_hi)
        return 0;
    return (a_lo < b_hi) && (b_lo < a_hi);
}

void pantheon_vram_log_layout(
    unsigned int fbp0, unsigned int fbp0_extent,
    unsigned int fbp1, unsigned int fbp1_extent,
    unsigned int zbp, unsigned int zb_extent,
    unsigned int tbp, unsigned int tex_extent,
    int has_texture)
{
    unsigned int hi0 = fbp0 + fbp0_extent;
    unsigned int hi1 = fbp1 + fbp1_extent;
    unsigned int zhi = zbp + zb_extent;
    unsigned int thi = has_texture ? (tbp + tex_extent) : 0u;

    printf(
        "pantheon_vram: FBP0=%u..%u (%uw) FBP1=%u..%u (%uw) ZBP=%u..%u (%uw)",
        fbp0, hi0, fbp0_extent,
        fbp1, hi1, fbp1_extent,
        zbp, zhi, zb_extent);

    if (has_texture)
        printf(" TBP=%u..%u (%uw)", tbp, thi, tex_extent);
    else
        printf(" TBP=(none)");

    {
        unsigned long used = (unsigned long)fbp0_extent + fbp1_extent + zb_extent;
        if (has_texture)
            used += tex_extent;
        printf(" words_used~=%lu remain~=%lu (extent_sum_noncontiguous)\n",
               used,
               (unsigned long)PANTHEON_VRAM_MAX_WORDS > used
                   ? (unsigned long)PANTHEON_VRAM_MAX_WORDS - used
                   : 0ul);
    }

    if (pantheon_vram_ranges_overlap(fbp0, hi0, fbp1, hi1))
        printf("pantheon_vram: WARNING FBP0 overlaps FBP1\n");
    if (pantheon_vram_ranges_overlap(fbp0, hi0, zbp, zhi))
        printf("pantheon_vram: WARNING FBP0 overlaps ZBP\n");
    if (pantheon_vram_ranges_overlap(fbp1, hi1, zbp, zhi))
        printf("pantheon_vram: WARNING FBP1 overlaps ZBP\n");
    if (has_texture) {
        if (pantheon_vram_ranges_overlap(fbp0, hi0, tbp, thi))
            printf("pantheon_vram: WARNING texture overlaps FBP0\n");
        if (pantheon_vram_ranges_overlap(fbp1, hi1, tbp, thi))
            printf("pantheon_vram: WARNING texture overlaps FBP1\n");
        if (pantheon_vram_ranges_overlap(zbp, zhi, tbp, thi))
            printf("pantheon_vram: WARNING texture overlaps ZBP\n");
    }
}
