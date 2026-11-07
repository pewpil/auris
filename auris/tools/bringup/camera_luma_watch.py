"""Sample the live camera stream and preserve low-rate brightness telemetry.

Purpose: Record whether the Pi camera feed remains dark or changes during
bring-up without storing images.
Inputs: An MJPEG camera URL, sample interval and duration in seconds, and an
output path.
Outputs: One UTC-stamped JSON object per sample with image dimensions and luma
statistics on a 0–255 grayscale scale; this is operational telemetry, not
evidence of camera or marker-tracking accuracy.
"""

from __future__ import annotations

import argparse
import json
import math
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Sequence
from urllib.error import URLError
from urllib.request import Request, urlopen

import cv2
import numpy as np


MAX_RESPONSE_BYTES = 2_000_000
READ_CHUNK_BYTES = 8_192


def fetch_grayscale_frame(url: str, timeout_s: float) -> tuple[np.ndarray, int]:
    """Decode the first JPEG frame from one live MJPEG response.

    Args:
        url: HTTP URL for the MJPEG camera stream.
        timeout_s: Maximum seconds to wait for a complete JPEG frame.

    Returns:
        A two-dimensional uint8 grayscale image in pixel coordinates and the
        encoded JPEG size in bytes.

    Raises:
        URLError: If the camera endpoint is unreachable or times out.
        ValueError: If no decodable JPEG frame arrives within the byte limit.
    """
    request = Request(url, headers={"Cache-Control": "no-cache"})
    payload = bytearray()
    with urlopen(request, timeout=timeout_s) as response:
        while len(payload) < MAX_RESPONSE_BYTES:
            chunk = response.read(READ_CHUNK_BYTES)
            if not chunk:
                break
            payload.extend(chunk)
            start = payload.find(b"\xff\xd8")
            if start < 0:
                continue
            end = payload.find(b"\xff\xd9", start + 2)
            if end < 0:
                continue
            jpeg = payload[start : end + 2]
            frame = cv2.imdecode(
                np.frombuffer(jpeg, dtype=np.uint8), cv2.IMREAD_GRAYSCALE
            )
            if frame is None:
                raise ValueError("camera stream contained an undecodable JPEG frame")
            return frame, len(jpeg)
    raise ValueError("camera stream did not deliver a complete JPEG frame")


def build_luma_record(
    frame: np.ndarray, jpeg_bytes: int, timestamp_utc: str
) -> dict[str, object]:
    """Summarize one grayscale frame without retaining its image data.

    Args:
        frame: Two-dimensional uint8 luma samples in pixel coordinates, range
            0–255.
        jpeg_bytes: Encoded frame size in bytes.
        timestamp_utc: ISO 8601 UTC time when the frame was sampled.

    Returns:
        JSON-safe frame dimensions in pixels and mean, median, 99th-percentile,
        and maximum luma values on the 0–255 scale.

    Raises:
        ValueError: If the image is not a two-dimensional grayscale frame.
    """
    if frame.ndim != 2 or frame.dtype != np.uint8:
        raise ValueError("camera frame must be a two-dimensional uint8 image")
    height_px, width_px = frame.shape
    p50, p99 = np.percentile(frame, [50, 99])
    return {
        "timestamp_utc": timestamp_utc,
        "frame_size_px": [int(width_px), int(height_px)],
        "jpeg_bytes": int(jpeg_bytes),
        "mean_luma_0_255": round(float(frame.mean()), 2),
        "p50_luma_0_255": round(float(p50), 2),
        "p99_luma_0_255": round(float(p99), 2),
        "max_luma_0_255": int(frame.max()),
    }


def parse_arguments(argv: Sequence[str] | None = None) -> argparse.Namespace:
    """Parse the camera URL, sample cadence, duration, and output path.

    Args:
        argv: Optional command-line arguments; ``None`` reads process arguments.

    Returns:
        Parsed settings; intervals, durations, and timeouts use seconds.

    Raises:
        SystemExit: If an argument is invalid or help is requested.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--camera-url", default="http://127.0.0.1:8081/camera")
    parser.add_argument("--interval-s", type=float, default=300.0)
    parser.add_argument("--duration-s", type=float, default=8 * 60 * 60)
    parser.add_argument("--timeout-s", type=float, default=5.0)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args(argv)
    timings = (arguments.interval_s, arguments.duration_s, arguments.timeout_s)
    if not all(math.isfinite(value) and value > 0 for value in timings):
        parser.error("interval, duration, and timeout must be finite and greater than zero")
    return arguments


def main(argv: Sequence[str] | None = None) -> int:
    """Append periodic camera-luma records until the requested window ends.

    Args:
        argv: Optional command-line arguments; ``None`` reads process arguments.

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
                frame, jpeg_bytes = fetch_grayscale_frame(
                    arguments.camera_url, arguments.timeout_s
                )
                record = build_luma_record(frame, jpeg_bytes, timestamp)
            except (URLError, TimeoutError, ValueError) as error:
                record = {"timestamp_utc": timestamp, "error": str(error)}
            output.write(json.dumps(record, separators=(",", ":"), allow_nan=False) + "\n")
            output.flush()
            next_poll = max(next_poll + arguments.interval_s, time.monotonic())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
