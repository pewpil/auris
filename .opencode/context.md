# Auris — opencode context

## Purpose

This file is loaded into opencode's context (system prompt) for every session on this project. It wires the canonical plan (`plan.md` at the repository root) and the project concept (`concept.md` in this directory) into the AI's context.

## Mandatory development mindset: Prototype → Production transition

Auris is developed in two phases: the **Prototype** (throwaway phones + Bluetooth earbuds feasibility rig) and the **Production system** (the real deliverable on specialized gear). The AI MUST keep this transition in mind for every decision, code change, or suggestion:

- **Keep** algorithms, data contracts, and data (coordinate math, HRTF/earcon library, YOLO model, SceneState schema, evaluation harness).
- **Rebuild** hardware-coupled layers in Production (camera capture, calibration, localization backend).
- **Never** let Prototype-only hacks (BT earbud latency, phone intrinsics, rolling shutter, low-res tuning) leak into Production assumptions. WiFi jitter handling is re-evaluated in Phase 4 if wireless remains the Production interconnect.
- Define data contracts before implementations (contract-first) so the hardware swap never touches the rest of the system.
- The Prototype is a thin vertical slice validating only the two core risks; it is not a parallel implementation of the Production system.

## Documents

- **`plan.md`** (repository root) — the canonical development plan: architecture, contract-first interfaces, phases, handoff checklist, evaluation, hardware roadmap, risks. Read it in full at the start of any planning or implementation task.
- **`concept.md`** (this directory) — what the project is: user, setup, hardware, system functions.

## Operating notes

- Python CV + Unity audio over UDP localhost (Python does all spatial math).
- Latency < 100 ms is an engineering target; the thesis does not study networking.
- The Production interconnect (wired vs wireless) is open; it is decided by measured end-to-end latency in Phase 4. Wireless remains a candidate as long as it meets the latency standard.
- Formal evaluation and user studies run only on the Production system.
