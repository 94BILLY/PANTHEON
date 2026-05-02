# PANTHEON 🏛️

**PANTHEON** is a deterministic, contract-driven rendering and data-orchestration framework engineered for the **PlayStation 2** architecture. Operating at the bare-metal layer, the platform enforces a rigorous manager–worker execution protocol: the **Emotion Engine (EE)** functions as a high-bandwidth stream orchestrator, sequencing 128-bit **DMA/VIF command chains** to **Vector Unit 1 (VU1)**.

By utilizing a **strict Path 1 pipeline**, Pantheon extracts maximum theoretical throughput from the silicon to achieve a **locked 60 FPS high-fidelity baseline**. All massive spatial transformations, VLIW microcode-based processing, and hardware-direct **XGKICK** operations are autonomously delegated to the isolated VU1 node, bypassing standard software bottlenecks to saturate the **Graphics Synthesizer (GS)**.

## **Core Architectural Invariants**

*   **Path 1 Data Plane Orchestration:** Pantheon rejects high-level wrappers to establish a direct conductor-node protocol. The **Emotion Engine** manages high-level world logic and world-sectoring while the **VU1** operates as a standalone geometry processor via the **VIF1 interface**.
*   **VLIW Execution Parallelism:** A bespoke microprogram (`shader.vsm`) facilitates dual-issue floating-point and integer math, executing two instructions per clock cycle to perform high-speed matrix multiplication and perspective division in parallel.
*   **Deterministic Memory Contract:** The framework is hardened by an **explicit 4MB strategic VRAM map**, enforcing strict **32-bit word-aligned** page (8KB) and block (256-byte) boundaries to facilitate zero-latency data transfers and prevent memory overlap.
*   **Automated Asset Telemetry:** An offline "Cruncher" pipeline (`hrc2ps2.py`) digests native **Softimage 3D (.hrc)** hierarchy data, enforcing **128-bit Quadword alignment** and W-axis padding to ensure hardware-safe data burst rates.
*   **Hardware-Level Safeguards:** The engine implements active **Near-Z culling** ($W \le 0.1f$) within the VU1 microcode to prevent coordinate saturation and maintains a strict color courier path to ensure chromatic stability across dynamic atmospheric transitions.
## **Operational Status: Phase 1 Baseline**

The framework has successfully achieved its **Phase 1 baseline** (git tag: `pantheon-base-60fps`), verifying the following performance matrix on **physical silicon**:

*   **Locked 60 FPS:** Sustained asynchronous rendering in a pure Path 1 environment.
*   **Geometry Stability:** Flicker-free Path 1 rendering of complex Softimage 3D world geometry and skydomes.
*   **Deterministic Telemetry:** GTA-style spherical orbital camera and XZ movement controls with precision deadzone clamping.
*   **Memory Discipline:** Verified zero-overlap **VRAM layout** audited via boot-time telemetry.

---

### **Platform Implementation Context**
While designed as a study in **asymmetric computational topologies**, this framework is currently implemented as a **Path 1 PlayStation 2 engine**. It treats legacy silicon like analog studio gear, bridging a 30-year technology gap to replicate the production-grade discipline of elite first-party development teams.


**Official Repository:** https://github.com/94BILLY/Pantheon  

**Author:** [94BILLY](https://github.com/94BILLY)

**Website:** https://www.94BILLY.com



 
