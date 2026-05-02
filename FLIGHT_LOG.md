# FLIGHT LOG

---

**Day 1 — 3/23/2026**

Ditched Path 3. CPU-heavy, slow, and it's what everyone does.
Path 1 is DMA-driven async VU1 rendering. That's the route.

Built the data contract. PantheonVertex + PantheonVIFHeader,
128-bit quadword aligned so the DMA controller can burst at
full speed. 32-bit color padding on vertices — Kojima/Rockstar
trick to prevent VIF pipeline stalls.

Built hrc2ps2.py. Takes a Softimage model from 1996, spits out
a 128-bit aligned C array ready for the GS and VU1.
Conveyor belt works.

Next: VIF1 render loop. Write the .vsm.

---

**Day 2 — 3/24/2026**

Breached the NT VM. Got raw ASCII geometry out of Softimage 3.92
via hrcConvert.exe. WSL file-pathing was a fight.

Cruncher handles quad splitting automatically.
64 quads → 112 triangles → 336 indices.
All 58 vertices formatted with W=1.0f. sphere_data.h done.

Committed to Path 1 fully. EE does not touch vertices.
Raw indices go over the bus. VU1 handles everything.

Next: VIF kick. DMA chain. XGKICK.

---

**Day 3 — 3/29/2026**

Conquered the assembler wall.

Wrote shader.vsm. Compiled with dvp-as, linked shader.o directly
into floor.elf. The two processors can talk.

GIF channel handles screen clear. VIF1 handles geometry.
Kept batches under 255 quadwords to stop the UNPACK from crashing.
Mapped vertex count to VU1 via ITOP so the microcode reads
array sizes dynamically.

Path 1 foundation is stable. CPU packages the data,
DMA moves it, VU1 executes the math.

---

**Day 4 — 4/14/2026**

Direct model injection from 1999 software into 2026 hardware.

Dropped the VU1 indexer. Brute-force streaming pipeline instead.
Built a 6x6 floor grid in Softimage, crunched it, got floor_data.h.
Engine unpacked 98 quadwords across VIF1 into VU1 local memory.
No crashes.

DMA bus stable. VU executing on Softimage geometry.

Next: debug the grey screen. Build the skydome. Grid chunking.

---

**Day 5 — 4/26/2026**

Killed the grey screen. Took two weeks.

Created .bat to simplify hrc → ASCII. Exported first real
edited asset from Softimage. Got a true Path 1 skydome running.
Kojima-style. Proper XGKICK to GS, no CPU involvement.

Issue: flickering blue screen on strict Path 1 mode.

Next: 6x6 Path 1 floor grid. Day/night math in skydome.
Expand floor to full map.

---

**Day 6 — 4/29/2026**

Path 1 engine solidified.
TrueSky math in the skydome.

Next: steady the floor grid. Lock the pipeline details.

---

**Day 7 — 5/2/2026**

Repo created. Made public.
Custom boot sequence added — 94BILLY STUDIOS.
Patched sky flicker regression after boot.
v1.0.0-beta.1 shipped; documentation pack lands in v1.0.0-beta.2.

Next: start building the actual world.
Era-accurate assets from Softimage 3D 1996.
The same tool Kojima used. The same tool Rockstar used.
Path 1 all the way.

---

*41 days. Seven sessions. Path 1, end-to-end and documented.*
