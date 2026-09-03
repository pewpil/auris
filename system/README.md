# Auris — System Software

Contract-first implementation of the Auris stack (plan: [README §4](../../README.md#4-data-contracts-contract-first-critical-for-the-transition);
architecture: [docs/architecture.md](../../docs/architecture.md)). All spatial
math lives in Python; Unity is a dumb audio renderer joined by the frozen
SceneState UDP schema.

## Layout

```
system/
├── python/                        CV pipeline + SceneState transport (this stage's workhorse)
│   ├── pyproject.toml             deps: numpy, opencv-python; dev: pytest, mypy
│   ├── auris/
│   │   ├── contracts/             ← THE contracts — both stages conform, hardware never leaks
│   │   │   ├── conventions.md     coordinate/unit bible (frames, azimuth sign rules, wire units)
│   │   │   ├── frame.py           Frame + CameraIntrinsics            (README §4.1)
│   │   │   ├── camera.py          CameraSource protocol                (§4.2 capture abstraction)
│   │   │   ├── localizer.py       ObjectObservation + ObjectLocalizer  (§4.2)
│   │   │   ├── head.py            HeadPose (6DoF) + HeadPoseEstimator  (§4.3)
│   │   │   ├── scene.py           SceneState v1 wire schema + codec    (§4.4, frozen in Phase 3)
│   │   │   └── audio.py           AudioSink protocol                   (§4.5)
│   │   ├── pipeline/              transform (world→head-relative math), scene model, runner
│   │   ├── synthetic/             hardware-free backends — full-stack runs before any device
│   │   ├── transport/             SceneStateStreamer (UDP, 30–60 Hz)
│   │   ├── capture/mjpeg.py       Phase-1 stub: phone MJPEG/RTSP (Prototype, RGB-only)
│   │   ├── perception/            Phase-1 stubs: MarkerLocalizer, head-marker pose
│   │   └── cli.py                 `auris stream --synthetic`, `auris calibrate`
│   └── tests/                     52 tests: transform goldens, wire round-trip, UDP smoke
└── unity/                         dumb renderer — scripts + editor-init docs (see its README)
```

## Backend swap map (transition-first)

| Contract | Prototype backend | Production backend |
|---|---|---|
| `CameraSource` | `capture/mjpeg.py` (phone MJPEG/RTSP, WiFi) | wired USB array / wireless edge nodes / optional depth |
| `ObjectLocalizer` | `perception/marker_localizer.py` (ARUCO solvePnP) | YOLO + one-shot registration → `TriangulatedLocalizer` |
| `HeadPoseEstimator` | `perception/head_marker_pose.py` (side-of-head pair) | markerless CV + headset-IMU fallback |
| `SceneState` | frozen here in Phase 3 | never changes — Unity untouched |
| `AudioSink` | Unity build → BT earbuds | Unity build → wired / 2.4 GHz headset |

`synthetic/` implements all three CV contracts hardware-free; it doubles as the
reference implementation for new backends.

## Quickstart

```bash
cd system/python
uv venv .venv && uv pip install --python .venv/bin/python numpy
.venv/bin/python -m auris stream --synthetic --port 5555   # streams until Ctrl+C
.venv/bin/python -m pytest                                  # 52 tests
.venv/bin/python -m mypy                                    # strict, 0 issues
ruff check .
```

Point Unity at the same port — steps in [`unity/README.md`](unity/README.md).

## SceneState v1 — freeze policy

The schema in `auris/contracts/scene.py` (50-byte header + 28+label bytes per
object, all little-endian, magic `AURS`) is **v1 draft**: additive changes are
allowed only with a `SCHEMA_VERSION` bump and a mirrored `SceneStateDecoder.cs`
update; the **freeze lands in Phase 3**, after which Unity must never see a
breaking change. The 84-byte golden fixture in `unity/README.md` pins the exact
byte layout on both sides.

## Standing rules honored here

- Contract-first: pipeline/runner code touches only protocol types — the Phase-1
  hardware backends slot in without edits above the `contracts/` line.
- Prototype-only hacks (phone intrinsics, WiFi jitter) are quarantined inside
  `capture/mjpeg.py` / `perception/` — never in `pipeline/` or `contracts/`.
- Latency < 100 ms is verified, not studied; the UDP streamer is the seam where
  Phase-3 instrumentation attaches.
