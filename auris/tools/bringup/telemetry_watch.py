"""Record low-rate Auris bring-up snapshots as JSON Lines.

Purpose: Preserve a compact read-only history of sensor availability, camera
tracking counters, and processing times while the compute unit runs.
Inputs: A `/state` URL, sample interval and duration in seconds, and output path.
Outputs: One UTC-stamped JSON object per poll; this is diagnostic sampling, not
the event-level in-use instrumentation required for study measurements.
"""

from __future__ import annotations

import argparse
import json
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Sequence
from urllib.error import URLError
from urllib.request import Request, urlopen


def fetch_state(url: str, timeout_s: float) -> dict[str, Any]:
    """Fetch and decode one live `/state` JSON snapshot.

    Args:
        url: HTTP URL of the compute unit's state endpoint.
        timeout_s: Maximum request duration, seconds.

    Returns:
        The decoded JSON object from `/state`.

    Raises:
        URLError: If the endpoint is unreachable or exceeds the timeout.
        ValueError: If the response is not a JSON object.
    """
    request = Request(url, headers={"Cache-Control": "no-cache"})
    with urlopen(request, timeout=timeout_s) as response:
        payload = json.loads(response.read().decode("utf-8"))
    if not isinstance(payload, dict):
        raise ValueError("state response must be a JSON object")
    return payload


def build_snapshot(state: dict[str, Any], timestamp_utc: str) -> dict[str, Any]:
    """Keep operational sensor and tracking fields from one state response.

    Args:
        state: Decoded `/state` object; angles are degrees and distances are
            metres in the head frame where the schema provides them.
        timestamp_utc: ISO 8601 UTC timestamp for the completed poll.

    Returns:
        A compact JSON-serializable diagnostic record without raw camera frames.
    """
    head = state.get("head") or {}
    pointer = state.get("pointer") or {}
    placement = state.get("placement") or {}
    camera = state.get("camera") or {}
    return {
        "timestamp_utc": timestamp_utc,
        "source": state.get("source"),
        "mock": state.get("mock"),
        "aligned": state.get("aligned"),
        "head": {
            "source": head.get("source"),
            "fresh": head.get("fresh"),
            "age_s": head.get("age_s"),
            "euler_deg": head.get("euler_deg"),
        },
        "pointer": {
            "available": bool(pointer),
            "source": pointer.get("source"),
            "schema": pointer.get("schema"),
            "imu_fresh": pointer.get("imu_fresh"),
            "tof_fresh": pointer.get("tof_fresh"),
            "sample_age_s": pointer.get("sample_age_s"),
            "calibrated": pointer.get("calibrated"),
            "flags": pointer.get("flags"),
            "accel_g": pointer.get("accel_g"),
            "gyro_dps": pointer.get("gyro_dps"),
            "temperature_c": pointer.get("temperature_c"),
            "range_status": pointer.get("range_status"),
            "range_age_ms": pointer.get("range_age_ms"),
            "range_m": pointer.get("range_m"),
            "button": pointer.get("button"),
            "matrix_head": pointer.get("matrix_head"),
        },
        "placement": {
            "baseline": placement.get("baseline"),
            "usable": placement.get("usable"),
            "distance_m": placement.get("distance_m"),
        },
        "serial_age_s": state.get("serial_age_s"),
        "camera": {
            "position_fresh": camera.get("position_fresh"),
            "dictionary_name": camera.get("dictionary_name"),
            "target_marker_ids": camera.get("target_marker_ids"),
            "marker_size_m": camera.get("marker_size_m"),
            "cube_size_m": camera.get("cube_size_m"),
            "marker_ids": camera.get("marker_ids"),
            "frame_count": camera.get("frame_count"),
            "detection_count": camera.get("detection_count"),
            "read_failures": camera.get("read_failures"),
            "frame_size_px": camera.get("frame_size_px"),
            "read_ms": camera.get("read_ms"),
            "detection_ms": camera.get("detection_ms"),
            "age_s": camera.get("age_s"),
            "reprojection_error_px": camera.get("reprojection_error_px"),
        },
    }


def parse_arguments(argv: Sequence[str] | None = None) -> argparse.Namespace:
    """Parse the endpoint, cadence, duration, and destination for monitoring.

    Args:
        argv: Optional argument sequence; `None` reads command-line arguments.

    Returns:
        Parsed settings; interval, duration, and timeout values use seconds.

    Raises:
        SystemExit: If an argument is invalid or help is requested.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--state-url", default="http://127.0.0.1:8081/state")
    parser.add_argument("--interval-s", type=float, default=30.0)
    parser.add_argument("--duration-s", type=float, default=8 * 60 * 60)
    parser.add_argument("--timeout-s", type=float, default=5.0)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args(argv)
    if arguments.interval_s <= 0 or arguments.duration_s <= 0 or arguments.timeout_s <= 0:
        parser.error("interval, duration, and timeout must be greater than zero")
    return arguments


def main(argv: Sequence[str] | None = None) -> int:
    """Append periodic state snapshots until the requested duration elapses.

    Args:
        argv: Optional argument sequence; `None` reads command-line arguments.

    Returns:
        Process exit code, zero after the monitoring window completes.

    Raises:
        OSError: If the output file cannot be opened or written.
    """
    arguments = parse_arguments(argv)
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    deadline = time.monotonic() + arguments.duration_s
    next_poll = time.monotonic()
    with arguments.output.open("a", encoding="utf-8") as output:
        while time.monotonic() < deadline:
            now = time.monotonic()
            if now < next_poll:
                time.sleep(min(next_poll - now, max(0.0, deadline - now)))
            timestamp = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
            try:
                state = fetch_state(arguments.state_url, arguments.timeout_s)
                record = build_snapshot(state, timestamp)
            except (URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
                record = {"timestamp_utc": timestamp, "error": str(error)}
            output.write(json.dumps(record, separators=(",", ":"), allow_nan=False) + "\n")
            output.flush()
            next_poll = max(next_poll + arguments.interval_s, time.monotonic())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
