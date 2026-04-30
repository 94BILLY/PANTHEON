# Pantheon Engine Architecture (MGS2-style baseline)

This repository follows a PS2 architecture where the EE schedules frame work and VU1 executes geometry for Path 1 rendering.

## Frame phases (authoritative order)

1. `phase_input_update`
   - Poll pad state, update player/camera state.
2. `phase_build_render_jobs`
   - Build deterministic per-frame render job table.
3. `phase_build_path1_packets`
   - Build VIF/GIF payloads from jobs using shared Path 1 contract.
4. `phase_submit_dma`
   - Submit GIF and VIF1 DMA packets.
5. `phase_present`
   - Flip framebuffer.

## Ownership split

- EE (`floor.c` main loop): scheduling, job assembly, DMA submission order.
- VU1 (`shader.vsm`): transform, clip handling, packed output, xgkick.
- Shared ABI: `pantheon_path1_contract.h` + `docs/path1_contract.md`.

## Asset readiness baseline

- SoftImage floor and skydome are the authored source of truth.
- Floor is normalized onto the world floor plane (`y=0`) for sandbox placement.
- Sky and floor run together in Path 1.
