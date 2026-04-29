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
