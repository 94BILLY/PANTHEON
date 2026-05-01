#include "pantheon_texture_host.h"
#include <draw_buffers.h>
#include <malloc.h>
#include <kernel.h>
#include <tamtypes.h>
#include <stdio.h>
#include <draw.h>
#include <packet.h>
#include <dma.h>
#include <graph_vram.h>
#include <gs_psm.h>

#include "pantheon_vram.h"

#ifndef PANTHEON_TEXTURE_PHASE
#define PANTHEON_TEXTURE_PHASE 0
#endif

#if PANTHEON_TEXTURE_PHASE

#define PTEX_W 64
#define PTEX_H 64

static void pantheon_fill_checker_rgba32(u32 *d, int w, int h)
{
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            d[y * w + x] = (((x ^ y) & 8) != 0) ? 0xFFFF8080u : 0xFF202080u;
        }
    }
}

void pantheon_host_texture_boot(texbuffer_t *tb, int *ok_out)
{
    packet_t *pkt;
    qword_t *q;
    u32 *pix;

    if (ok_out)
        *ok_out = 0;
    if (!tb)
        return;

    tb->width = PTEX_W;
    tb->psm = GS_PSM_32;
    {
        unsigned int tex_words = pantheon_vram_surface_extent_words(PTEX_W, PTEX_H, GS_PSM_32, GRAPH_ALIGN_BLOCK);
        tb->address = pantheon_vram_linear_alloc_words(tex_words, PANTHEON_VRAM_BLOCK_WORDS);
    }
    if (tb->address == 0u) {
        printf("pantheon_texture: pantheon_vram_linear_alloc_words failed\n");
        return;
    }
    tb->info.width = 0;
    tb->info.height = 0;
    tb->info.components = 0;
    tb->info.function = 0;

    pix = (u32 *)memalign(128, PTEX_W * PTEX_H * 4);
    if (!pix) {
        printf("pantheon_texture: memalign failed\n");
        return;
    }
    pantheon_fill_checker_rgba32(pix, PTEX_W, PTEX_H);
    FlushCache(0);

    pkt = packet_init(64, PACKET_NORMAL);
    if (!pkt) {
        free(pix);
        return;
    }
    q = pkt->data;
    /*
     * libdraw builds the PS2DMA + GIF packet for host->local upload (IMAGE path / TRX)
     * and pairs it with draw_texture_flush (GS TEXFLUSH).
     */
    q = draw_texture_transfer(q, pix, PTEX_W, PTEX_H, GS_PSM_32, (int)tb->address, PTEX_W);
    q = draw_texture_flush(q);
    FlushCache(0);
    dma_channel_send_chain(DMA_CHANNEL_GIF, pkt->data, q - pkt->data, 0, 0);
    dma_wait_fast();
    packet_free(pkt);
    free(pix);

    printf("pantheon_texture: upload OK TBP=%uw (64x64 PSMCT32) TEXFLUSH issued\n", tb->address);
    if (ok_out)
        *ok_out = 1;
}

#else /* !PANTHEON_TEXTURE_PHASE */

void pantheon_host_texture_boot(texbuffer_t *tb, int *ok_out)
{
    if (tb) {
        tb->address = 0;
        tb->width = 0;
        tb->psm = 0;
    }
    if (ok_out)
        *ok_out = 0;
}

#endif /* PANTHEON_TEXTURE_PHASE */
