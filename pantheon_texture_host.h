/*
 * pantheon_texture_host.h
 * -----------------------
 * Phase 2 stub — NOT compiled into floor.elf (Phase 1 baseline).
 * Not referenced in Makefile.world.
 *
 * Planned: host-side GS texture upload helpers (TBP allocation,
 * TRXPOS/TRXREG/TRXDIR packet construction, swizzled PSMCT32 upload
 * via PATH3 DMA). Will be wired in when Phase 2 STQ work begins.
 *
 * Do not link against this file until Phase 2 is active.
 */

#ifndef PANTHEON_TEXTURE_HOST_H
#define PANTHEON_TEXTURE_HOST_H

#include <draw_buffers.h>

#ifndef PANTHEON_TEXTURE_PHASE
#define PANTHEON_TEXTURE_PHASE 0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Optional vertical slice: allocate block-aligned TBP, upload 64x64 PSMCT32 via
 * libdraw draw_texture_transfer (GIF IMAGE/TRX path) + draw_texture_flush (TEXFLUSH).
 * When PANTHEON_TEXTURE_PHASE is 0 at compile time, this is a no-op.
 */
void pantheon_host_texture_boot(texbuffer_t *tb, int *ok_out);

#if defined(__cplusplus)
}
#endif

#endif /* PANTHEON_TEXTURE_HOST_H */
