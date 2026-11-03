# 3D debug view — AGENTS.md

Standing instructions for `3d/`. Read with [`../AGENTS.md`](../AGENTS.md) for the repository-wide sections.

> **Scope:** [§2](../AGENTS.md) (every function, class and non-obvious type carries a JSDoc block — this directory is TypeScript-free ES module JavaScript), [§3](../AGENTS.md) (the AI never commits), [§4](../AGENTS.md) (the view is a debugging instrument: nothing it renders is §4 evidence), [§5](../AGENTS.md) (never artificially wrap prose), and [§6](../AGENTS.md) (side notes) apply here unchanged. `3d/` is an ordinary directory in the superproject — same git history as the root, not a submodule.

## What this is

A live three.js view of the geometry the compute unit is using, served by the compute unit's `--viz` option when `3d/` sits beside its checkout. It exists to answer one question at a glance: **where is the sound being placed, and why?** The client is stateless apart from the server field kept in `localStorage`; every number it shows comes from the compute unit's `GET /state` snapshot, and no computation of its own upgrades a `?demo` guess into a hardware reading.

## Frames and drawing invariants

| Object | Meaning |
|---|---|
| Blue sphere with an orange cone | The head. The cone is the nose, on the head's `+X`, so it shows which way the listener faces. **The head is the anchor: it stays at the world centre.** |
| Orange cube with a bright tip | The pointer. Its long axis is the pointer's **aim axis** (`--aim-axis`, `+x` by default), so the tip shows where the beam leaves the body. Its position is the one the tracker reports. |
| Cyan beam | The ToF beam, from the drawn pointer to the landing. Its length is the measured range. |
| Green/pale sphere at the landing | The hit. It turns red as the gain falls to zero. |
| Dashed white line | Head → landing. Identical to the sound direction whenever the drawn origin agrees with the audio origin. |
| Solid grey line | Head → pointer, the tracked displacement the gain is *not* using. |

- **Everything drawn lives in the head body frame**: `+X` forward, `+Y` left, `+Z` up, metres and degrees — the same frame `audition.placement` publishes and the terminal prints. A sign flip here is a sign flip there; fix it at the source, never in the page.
- **The drawn pointer sits at `tracked.origin_m`** (the inertially tracked displacement, `cu/audition/tracking.py`) when the snapshot carries one, and falls back to `placement.origin_m` — the audio origin — when it does not (head source disabled, or nothing integrated yet). With neither, the origin is the world centre.
- **The beam and landing are computed from the drawn origin** (`hit = origin + beam · range_m`), not from `placement.hit_m`. The drawn landing answers "where would the beam land if the drawn pointer were where the tracker says?" — its difference from the audible hit is the parallax error the camera fix exists to remove, made visible.
- **The HUD always reports the audio's numbers** — placement, gain, pan, baseline — even while the drawn geometry disagrees with them. The `tracked` row is the only HUD field about the tracker (radius plus `hold`/`stale`/`clamp`); the drawn-versus-audible split is deliberate, so never "fix" one to match the other.
- The camera auto-fits **once** on the first usable hit; after that `f` is the only refit. A moving pointer must not drag the view around.

## The data contract

The page polls `GET /state` every 50 ms. `audition.viz.build_state` in `cu/` builds the snapshot and is its sanitiser: every number is finite, rounded and JSON-safe before this page sees it, so the client needs no NaN handling. **The two sides must be changed in one pass**: a field added, renamed or re-shaped here touches `build_state`, this table, `demoState`, and the HUD/legend together — the `?demo` path exists exactly so a contract change can be rendered the same day it is made.

| Key | Meaning |
|---|---|
| `t` | Seconds since the pipeline started. |
| `aligned`, `muted` | Yaw alignment done, and whether the renderer is silent. |
| `config.max_range_m`, `config.full_scale_deg` | The gain and pan scales in use. |
| `head.euler_deg`, `head.fresh` | Head roll/pitch/yaw in degrees, and whether the sample was fresh. |
| `pointer.matrix_head` | Pointer body-to-head rotation, nine values, **row-major**. |
| `pointer.range_m`, `pointer.button` | ToF range and button state at that update. |
| `placement.origin_m` | The audio placement's pointer origin in the head frame (camera fix or default baseline). The *drawn* pointer ignores it when `tracked` is present. |
| `placement.beam` | Unit beam direction in the head frame. |
| `placement.hit_m` | The audible hit point in the head frame, or `null`. |
| `placement.distance_m`, `gain`, `pan` | The values the audio used. |
| `placement.baseline` | `camera`, `hold`, `default` or `assumed`. |
| `placement.usable` | False when there was no valid range, so the beam and landing are hidden. |
| `tracked.origin_m` | The inertially tracked pointer displacement from the head, metres, head frame, or `null` when disabled or not yet seeded. The pointer cube is drawn here. |
| `tracked.speed_mps` | Magnitude of the tracked relative velocity, m/s. |
| `tracked.holding`, `tracked.pointer_stale`, `tracked.clamped` | Tracker status flags, shown in the `tracked` HUD row. |
| `audio.gain`, `audio.pan` | What was actually sent to the renderer, after the button gate. |
| `camera.distance_m`, `camera.age_s` | Marker distance and fix age, or `null`. |

A snapshot with `placement: null` means the pipeline could not place anything yet: not aligned, no pointer rows, or no fresh head sample. The page keeps the last drawing and reports `no data` until the next successful poll, which is what makes a compute-unit restart visible without reloading.

## Running and checking it

The compute unit serves the page itself when `3d/` sits next to `cu/`:

```bash
python -m audition --head-source serial --pulse-hz 2 --viz
```

`--viz` without a value uses port **8081** (`--viz 9000` picks another). Open `http://<pi-address>:8081/` on the laptop. The banner prints the port; the `/state` endpoint is always available even when the page cannot be found.

On the current Pi the view lives at `/home/user/Projects/3d`, synced next to the compute unit's checkout at `/home/user/Projects/system`. A deployment with only `cu/` still gets the `/state` endpoint; serve the page from the laptop instead and point it at the Pi:

```bash
python3 -m http.server 8000 --directory 3d
# then open  http://localhost:8000/?server=http://<pi-address>:8081
```

Without hardware, the whole thing runs on one machine:

```bash
cd cu
PYTHONPATH=. .venv/bin/python -m audition \
    --pointer-source synthetic --head-source synthetic \
    --no-camera --align none --audio null --viz
```

And the page alone can be checked with no server at all:

```
http://localhost:8000/?demo
```

`?demo` animates synthetic data in the browser using the same geometry as the pipeline, which is the quickest way to confirm the page itself still renders. Controls: **drag** to orbit, **wheel** to zoom, **`f`** to refit.

## How changes are verified

- **`node --check viz.js`** catches module syntax before anything loads.
- **The `?demo` headless render** confirms the scene draws: the pointer cube should wander off the beam axis (it rides the tracked origin, which demo data perturbs), the beam should leave the cube, and the HUD's `tracked` row should populate.
- **`cu/tests/test_viz.py`** holds the server side of the contract: snapshot serialisation including the `tracked` field, JSON safety, and the HTTP `/state` endpoint. Run the `cu/` suite with any `build_state` change.
- A change that only moves pixels needs the two render checks; a contract change needs the render checks *and* the `cu/` suite.

## Known limitations

- **The view is a debugging drawing, not evidence.** It reproduces what `/state` carries; a renderer bug can lie about geometry, and §4 supplies nothing here. The audible path's provenance lives in `cu/audition/placement.py` and the pipeline's terminal lines.
- **The tracked position dead-reckons and drifts.** Its guards (learned rest reading, priming, velocity leak, staleness freeze, 2 m clamp) and its known limits are documented in the drawn-pointer section of [`../cu/AGENTS.md`](../cu/AGENTS.md); the drawn pointer is expected to diverge from any hand-measured distance. The camera fix or the default baseline still owns every distance the audio uses.
- **Headless rendering needs software GL.** The bench Chrome runs with `--use-angle=swiftshader --enable-unsafe-swiftshader`; a machine with no GPU still renders, just slowly.

## Files

| File | Role |
|---|---|
| `index.html` | Page shell, panel, legend and import map. |
| `viz.js` | Scene, state application, polling and demo data. |
| `vendor/three.module.min.js` | three.js r160, pinned and vendored so the bench network needs no internet. |
| `vendor/OrbitControls.js` | OrbitControls from the same release. |

The server side lives in `cu/audition/viz.py`; the CLI options are `--viz [PORT]` and `--viz-root` in `cu/audition/cli.py`.
