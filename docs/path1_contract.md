# Pantheon Path 1 Contract (MGS2-style discipline)

This file freezes the EE <-> VU1 <-> GS runtime contract used by Pantheon Path 1.

## Scope

- Applies to `floor.c` packet build path and `shader.vsm` VU1 microprogram.
- Applies to authored SoftImage floor/skydome mesh submissions.
- Applies to normal render mode (not CPU overlay debug shortcuts).

## VU1 Data-Memory Layout (QW addresses)

- `PATH1_VU_CONST_PERSPECTIVE_ADDR = 0`
- `PATH1_VU_CONST_SCREEN_ADDR = 2`
- `PATH1_VU_CONST_MVP_ADDR = 4`
- `PATH1_VU_INPUT_BASE = 8`
- `PATH1_VU_GIFTAG_ADDR = 240`
- `PATH1_VU_OUTPUT_BASE = 241`
- `PATH1_VU_XGKICK_ADDR = 240` (must equal giftag addr)

## Per-Vertex Payload Contract

Each vertex is exactly `PATH1_VU_QW_PER_VERT = 3` quadwords:

1. Position (`x,y,z,w`)
2. Courier color (`RGBA packed into .r float`, `Q=1.0` in `.g`)
3. STQ (`s,t,tex_q,pad`)

## GIF PACKED Register Order

Current packed order is fixed:

- `GIF_REG_RGBAQ`
- `GIF_REG_ST`
- `GIF_REG_XYZ2`

## EE Responsibilities

- Build deterministic render jobs in fixed pass order.
- Build VIF/GIF packets that exactly match this contract.
- Submit VIF1 DMA without mutating per-pass memory layout.

## VU1 Responsibilities

- Transform/clip/projection from unpacked input payload.
- Pack output in contract register order.
- Issue `xgkick` from `PATH1_VU_XGKICK_ADDR`.

## Do Not Change Without Coordinated Update

The following are contract-critical and must be changed together (EE + VU):

- Any VU memory address constants.
- Per-vertex QW count or field order.
- GIF packed register list/order.
- Screen/near constants semantics.

## Frame Phases (EE Scheduler Contract)

Pantheon frame flow is treated as fixed phases:

1. Input/update
2. Render-job build
3. Path1 packet build
4. DMA submit
5. Present

This mirrors MGS2-style discipline: EE schedules, VU1 executes geometry work.
