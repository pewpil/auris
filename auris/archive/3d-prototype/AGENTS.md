# 3D debug view — AGENTS.md

Standing instructions for `3d/`. Read with [`../AGENTS.md`](../AGENTS.md) for the repository-wide sections.

> **Scope:** [§2](../AGENTS.md) (every function, class and non-obvious type carries a JSDoc block — this directory is TypeScript-free ES module JavaScript), [§3](../AGENTS.md) (the AI never commits), [§4](../AGENTS.md) (the view is a debugging instrument: nothing it renders is §4 evidence), [§5](../AGENTS.md) (never artificially wrap prose), and [§6](../AGENTS.md) (side notes) apply here unchanged. `3d/` is an ordinary directory in the superproject — same git history as the root, not a submodule.

## What this is

A live three.js view of the geometry the compute unit is using, served by the compute unit's `--viz` option when `3d/` sits beside its checkout. It exists to answer one question at a glance: **where is the sound being placed, and why?** The client is stateless apart from the server field kept in `localStorage`; every number it shows comes from the compute unit's `GET /state` snapshot, and no computation of its own upgrades a `?demo` guess into a hardware reading.

## Frames and drawing invariants

| Object | Meaning |
|---|---|
| Blue sphere with an orange cone | The head. The cone is the nose, on the head's `+X`, so it shows which way the listener faces. **The head is the anchor: it stays at the world centre.** |
| Orange cube with a bright tip | The raygun. Its long axis is the pointer's **aim axis** (`--aim-axis`, `+x` by default), so the tip shows the aim direction. While a tag fix is fresh, filtered camera cube rotation is authoritative for attitude and aim; the IMUs are ignored for raygun orientation. The pointer and head IMUs carry relative orientation only after vision goes stale. Its position follows the filtered camera-measured cube centre first, then the inertial fallback. |
| Cyan beam | The ToF beam, from the drawn pointer to the landing. Its length is the measured range. |
| Green/pale sphere at the landing | The hit. It turns red as the gain falls to zero. |
| Dashed white line | Head → landing. Identical to the sound direction whenever the drawn origin agrees with the audio origin. |
| Solid grey line | Head → pointer, the tracked displacement the gain is *not* using. |

- **Head-registered geometry lives in the head body frame**: `+X` forward, `+Y` left, `+Z` up, metres and degrees — the same frame `audition.placement` publishes and the terminal prints. When no head reference exists, the dashboard may show an explicitly labeled pointer-local orientation and ToF endpoint from an arbitrary display origin; that view has unregistered yaw and unknown pointer-to-head position, so it is not a head-frame pose. A sign flip here is a sign flip there; fix it at the source, never in the page.
- **The drawn raygun and aim use the camera-observed cube pose first** while an ArUco fix is no more than 250 ms old: `camera.position_head_m` supplies position and `camera.rotation_head` supplies filtered attitude and beam direction. Each distinct fresh camera rotation also reanchors the IMU yaw offset; after vision goes stale, the IMUs propagate attitude from the most recent visual alignment instead of the startup alignment. The last camera position is held briefly, then `placement.origin_m` is used. With none available, the origin is the world centre.
- **The beam and landing are computed from the drawn origin** (`hit = origin + beam · range_m`), not from `placement.hit_m`. The drawn landing answers "where would the beam land if the drawn pointer were where the tracker says?" — its difference from the audible hit is the parallax error the camera fix exists to remove, made visible.
- **Pointer-only fallback is diagnostic**: `pointer.matrix_pointer_inertial` is the pointer-body-to-filter-world orientation, row-major, from the pointer's AHRS whose yaw starts at zero and is unregistered to the head. The page may rotate the raygun and draw a fresh ToF endpoint from a zero display anchor when no fresh shared frame exists, but it must hide the head and state that neither the displayed origin nor endpoint is a head-relative position. If an earlier head sample exists but is stale, its last head-registered transform is labeled held stale.
- **The HUD always reports the audio's numbers** — placement, gain, pan, baseline — even while the drawn geometry disagrees with them. The `tracked` row is the only HUD field about the tracker (radius plus `hold`/`stale`/`clamp`); the drawn-versus-audible split is deliberate, so never "fix" one to match the other.
- The camera auto-fits **once** on the first usable hit; after that `f`, **Fit scene**, or **Recenter** explicitly refit it. A moving pointer must not drag the view around.

## The data contract

The page targets a 20 ms start-to-start interval (50 Hz) for `GET /state`, counting request and response time toward the interval and never allowing overlapping polls; slower links are naturally limited by response time. Its HTTP metric reports both the latest fetch duration and the observed start-to-start cadence over a five-second window. It shows the annotated MJPEG camera feed at `GET /camera` on the same port. `audition.viz.build_state` in `cu/` builds the snapshot and is its sanitiser: every number is finite, rounded and JSON-safe before this page sees it, so the client needs no NaN handling. **The two sides must be changed in one pass**: a field added, renamed or re-shaped here touches `build_state`, this table, `demoState`, and the HUD/legend together — the `?demo` path exists exactly so a contract change can be rendered the same day it is made.

The camera card samples its visible MJPEG frame locally at 2 Hz and displays approximate mean brightness on a 0–255 scale with a proportional meter. Mean luma below 40 triggers an operator hint to check the lens cover, camera aim, and room lighting; when frames are advancing without a fresh tag pose, a separate hint tells the operator to aim a configured cube face at the camera and bring it closer. These are operator aids, not ArUco quality thresholds. The page estimates capture rate from `camera.frame_count` and marks capture stalled after one second without a new frame; it suppresses the brightness reading while stalled so a frozen image cannot look like a current lighting measurement. These client-side diagnostics are not part of `/state` and are not evidence of camera or tracker quality.

The dashboard exposes the published head and pointer orientations, ToF range, button state, camera fix, placement and audio values beside the 3D view. The head card reports the selected input and freshness, and explains when the pointer's USB debug-table stream contains no head `H1` records. It derives an observed yaw-change rate from the pointer matrix over up to 60 seconds only while the camera pose is stale; a fresh ArUco pose clears the IMU history and displays `ArUco locked`. This rate includes real turns and inertial drift, so it is not a gyro-bias measurement. Its pause, fit, connection and demo controls affect the browser only; the current Pi endpoint is read-only and does not accept hardware commands.

The top status badge separates HTTP availability from tracking readiness: `offline` and `timeout` mean `/state` could not be read, while `vision locked`, `IMU fallback`, `pointer only`, `head only`, `searching tags`, `vision stale`, `pointer stale`, `camera stalled`, and `waiting for sensors` describe the latest sensor tier. A pointer snapshot counts as live only when both its IMU and ToF freshness flags are true and any reported sample age is at most 250 ms; a snapshot without freshness fields is reported as unknown rather than assumed live. `vision locked` requires a fresh camera pose and fresh pointer IMU/ToF data.

On narrow screens the 3D scene stays full-bleed, the live sensor panel becomes a bottom drawer with tap and swipe controls, the camera preview expands for tag inspection, and the connection editor and view actions use labeled icon buttons.

| Key | Meaning |
|---|---|
| `t` | Seconds since the pipeline started. |
| `aligned`, `muted`, `mock` | Yaw alignment done, whether the renderer is silent, and whether either IMU stream is synthetic. |
| `source` | Optional pointer input source reported by the compute unit, such as `pointer-usb-table`; the dashboard shows it verbatim and warns when the API omits it. |
| `config.max_range_m`, `config.full_scale_deg` | The gain and pan scales in use. |
| `head.euler_deg`, `head.fresh` | Head roll/pitch/yaw in degrees, and whether the sample was fresh. |
| `pointer.matrix_head` | Pointer body-to-head rotation, nine values, **row-major**; the dashboard labels it held when its last head sample has aged stale. |
| `pointer.matrix_pointer_inertial` | Pointer body-to-filter-world rotation, nine values, **row-major**; diagnostic orientation only, with yaw unregistered to the head. |
| `pointer.range_m`, `pointer.button` | ToF range and button state at that update. |
| `placement.origin_m` | The audio placement's pointer origin in the head frame (camera fix or default baseline). The *drawn* pointer ignores it when `tracked` is present. |
| `placement.beam` | Unit beam direction in the head frame. |
| `placement.hit_m` | The audible hit point in the head frame, or `null`. |
| `placement.distance_m`, `gain`, `pan` | The values the audio used; pan is unitless from -1 (full left) to +1 (full right). |
| `config.max_range_m` | Full-scale distance for the ToF and head-to-hit range meters. |
| `placement.baseline` | `camera`, `hold`, `default` or `assumed`. |
| `placement.usable` | False when there was no valid range, so the beam and landing are hidden. |
| `tracked.origin_m` | The inertially tracked pointer displacement from the head, metres, head frame, or `null` when disabled or not yet seeded. The pointer cube is drawn here. |
| `tracked.speed_mps` | Magnitude of the tracked relative velocity, m/s. |
| `tracked.holding`, `tracked.pointer_stale`, `tracked.clamped` | Tracker status flags, shown in the `tracked` HUD row. |
| `audio.gain`, `audio.pan` | What was actually sent to the renderer, after the button gate. |
| `camera.distance_m`, `camera.age_s` | Fused camera-to-pointer-cube-centre distance and fix age, or `null`. |
| `camera.position_head_m` | Last camera-observed pointer cube centre in the head frame, metres; retained for display after it becomes stale. |
| `camera.position_fresh` | True while the latest camera pose is no more than 250 ms old and can anchor current position and orientation. |
| `camera.rotation_head` | Camera-observed raygun cube-to-head rotation, nine values, **row-major**; prefer it while fresh, then use the pointer IMU matrix. |
| `camera.marker_ids` | ArUco IDs contributing to the current fused cube-centre position. |
| `camera.frame_count`, `camera.detection_count` | Captured frames and frames with an accepted cube pose, cumulative since pipeline startup; the page smooths its capture-rate estimate over five seconds and detects a one-second frame-counter stall. |
| `camera.reprojection_error_px` | Reprojection RMS for the representative tag observation, pixels; lower generally indicates a better image fit. |
| `camera.calibration_source`, `camera.frame_size_px`, `camera.read_failures` | Calibration provenance, actual image dimensions in pixels, and cumulative frame/detection errors. |
| `camera.read_ms`, `camera.detection_ms` | Latest blocking camera-read time and ArUco detect-plus-pose time, milliseconds; compare these with the measured camera FPS to identify the tracking bottleneck. |

A snapshot with `placement: null` means the pipeline could not compute head-relative placement yet: not aligned, no pointer rows, or no fresh head sample. The page may still show the pointer-local inertial orientation and a ToF endpoint when those sensors are fresh, with the head hidden and frame limitations labeled; otherwise it keeps the last drawing and reports that no registered pose is available until a later snapshot.

## Running and checking it

The compute unit serves the page itself when `3d/` sits next to `cu/`:

```bash
python -m audition --head-source serial --pulse-hz 2 --viz
```

`--viz` without a value uses port **8081** (`--viz 9000` picks another). Open `http://<pi-address>:<viz-port>/` on a laptop or phone connected to the same reachable network; use the Pi's Wi-Fi address for Wi-Fi clients or its Ethernet address for wired clients. The dashboard, `/state` endpoint, and proxied annotated camera feed are served from the same port, so the dashboard's camera view works through a temporary HTTPS tunnel as well as the local network.

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

`?demo` animates synthetic data in the browser using the same geometry as the pipeline, which is the quickest way to inspect the page without a server. Controls: **drag** to orbit, **wheel** to zoom, **`f`**, **Fit scene**, or **Recenter** to refit, **Pause updates** to freeze the current snapshot, and **Start demo** to switch to synthetic snapshots. The dashboard requires a same-origin compute-unit server or an entered Pi URL when hosted elsewhere.

To exercise the real HTTP fetch path without hardware, run `python 3d/mock_server.py --delay-ms 80` from the repository root and open `http://127.0.0.1:8082/`. The mock server serves this page and an animated `/state` response; **HTTP fetch RTT** includes the configured response wait, browser/network overhead, and JSON parsing, so it is not a radio-latency measurement. Run `python 3d/mock_server.py --delay-ms 80 --self-test` to confirm the page is served and measure three loopback HTTP requests from the same process.

The pointer firmware's compact 12-column USB table can drive the same page directly on the Pi with `python3 mock_server.py --host 0.0.0.0 --port 8082 --serial-port /dev/ttyUSB0 --baud 115200`; it reports actual pointer orientation, button and ToF data, while explicitly leaving head IMU and ArUco values unavailable. Full head-frame placement still requires the bridge's interleaved `P1` and `H1` stream at 460800 baud through the compute-unit pipeline.

For a text dashboard in Windows Terminal without Python, run `.\3d\terminal_view.ps1` from the repository root; it defaults to the Pi's current Wi-Fi address and port 8081. On this PC, use `-Url http://192.168.77.2:8081/state -SourceIp 192.168.77.1` for direct Ethernet, pass `-Url http://<pi-address>:<viz-port>/state` after the address or viz port changes, and `-Once` for a single snapshot. The Python alternative is `python .\3d\terminal_view.py --url http://<pi-address>:<viz-port>/state` when Python is installed. Both poll the same read-only snapshot, render ToF and head-to-hit distances as fixed-scale bars, show cumulative camera frames, accepted cube-pose frames, read failures, and latest read/detection times, and distinguish sensor availability from yaw alignment.

On this Windows PC, a browser may route the Pi Ethernet address through Wi-Fi instead of the direct cable. Run `.\3d\open_ethernet_view.ps1` from the repository root; it checks or starts the loopback-only proxy, waits for the Pi health endpoint over Ethernet, then opens `http://127.0.0.1:8781/`. Run it with `-NoBrowser` to check the proxy and Pi health without opening a window. To start the proxy directly without checking or opening a browser, run `python .\3d\ethernet_proxy.py --source-ip 192.168.77.1`. A phone on the Pi's Wi-Fi can open the Pi's Wi-Fi URL directly.

For low-rate operational history during live bring-up, run `python3 telemetry_watch.py --state-url http://127.0.0.1:8081/state --interval-s 30 --duration-s 28800 --output /home/user/auris-bringup-watch.jsonl` on the Pi. It appends one compact JSON Lines snapshot per interval and records request errors without issuing sensor or device commands. This is a bring-up aid, not event-level in-use instrumentation and not evidence of tracking accuracy or latency quality.

To save a camera-brightness trace when no browser is open, run `/home/user/Projects/system/.venv/bin/python camera_luma_watch.py --camera-url http://127.0.0.1:8081/camera --interval-s 300 --duration-s 28800 --output /home/user/auris-camera-luma.jsonl` from `/home/user/Projects/3d`. It records one decoded frame's dimensions and mean, median, 99th-percentile, and maximum luma on a 0–255 scale without retaining image data. This is operational scene telemetry, not evidence of camera or tracking quality.

To summarize an overnight bring-up window, run `python3 analyze_telemetry.py --state-log /home/user/auris-bringup-watch.jsonl --camera-log /home/user/auris-camera-luma.jsonl --since-utc 2026-11-06T21:20:00+00:00 --report /home/user/auris-bringup-report.md` from `/home/user/Projects/3d`; choose the cutoff matching the current recorder run so older appended rows do not leak into its summary. The report includes sensor availability, ToF range, camera counters and timings, counter drops, latest and peak detection counts, latest and window-observed marker IDs, luma history, and pointer aim-azimuth change; any drift interpretation is conditional on the pointer having stayed physically still, and the report does not establish tracking accuracy or study latency.

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
| `bringup-log.md` | Dated Raspberry Pi sensor and service status snapshot with the next physical bring-up actions. |
| `mock_server.py` | Local synthetic `/state` server for real HTTP fetch-delay visualization. |
| `ethernet_proxy.py` | Loopback-only Windows proxy that binds Pi dashboard requests to the Ethernet adapter. |
| `open_ethernet_view.ps1` | One-command Windows launcher for the direct Ethernet dashboard path. |
| `terminal_view.py` | Python terminal dashboard for the live `/state` endpoint. |
| `terminal_view.ps1` | Windows PowerShell terminal dashboard with no Python dependency. |
| `telemetry_watch.py` | Read-only, low-rate `/state` snapshots to JSON Lines for bring-up. |
| `camera_luma_watch.py` | Read-only, low-rate camera brightness snapshots without storing frames. |
| `analyze_telemetry.py` | Read-only Markdown summary of state and camera-luma JSON Lines bring-up logs. |
| `vendor/three.module.min.js` | three.js r160, pinned and vendored so the bench network needs no internet. |
| `vendor/OrbitControls.js` | OrbitControls from the same release. |

The server side lives in `cu/audition/viz.py`; the CLI options are `--viz [PORT]` and `--viz-root` in `cu/audition/cli.py`.
