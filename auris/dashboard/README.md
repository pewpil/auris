# Auris navigation dashboard

Serve this directory through the compute unit’s `--viz-root` option. The dashboard reads same-origin `GET /state` and the MJPEG stream at `GET /camera`; it sends no device commands. Opening `index.html` directly does not provide these routes.

Run `./run_auris.sh` on the Pi for the existing IMU/ToF configuration. Set `AURIS_CAMERA=1 ./run_auris.sh` to omit `--no-camera` once the compute-unit camera configuration is ready. The launcher still selects null audio and no alignment; use the compute unit’s configured navigation launch command for audio and usable fused placement. The compute-unit source is not present in this checkout.

The connection profile is an operator-selected label, not network-interface detection. Head and probe cards show published Euler angles, a yaw arrow, acceleration converted from g to m/s² using 9.80665, and gyroscope readings in °/s. Missing raw head measurements display “Not reported”; they are not inferred from orientation. All XYZ sensor values retain their published sensor frame.

Tag rows use `camera.target_marker_ids` and observed `camera.marker_ids`. Visibility requires progressing camera frames and `camera.position_fresh`; absent or stalled telemetry displays unknown. This is accepted-pose visibility, not an independent raw marker detection measurement.

The orbitable scene uses metres in the head frame (+X forward, +Y left, +Z up). It draws only a usable `placement` containing `origin_m`, `beam`, and `range_m`. The hit is `origin_m + normalize(beam) * range_m`; its distance to the head is the norm of that point. No position is inferred from IMU angles alone. The probe box shows aim direction; it does not represent full probe roll. Lost API access hides placement geometry and marks retained IMU values stale.

The three.js dependencies are local copies of the repository’s archived prototype assets; see `vendor/README.md`. No external CDN is required.

Verification: [SIM] synthetic browser-independent rendering checks cover unit conversion, orientation extraction, tag visibility, and disconnect behavior. These checks do not establish hardware accuracy, tracking hand-off behavior, or live camera operation.
