"""Display the Auris live sensor snapshot as a compact terminal dashboard.

Run this on a Windows PC or another machine that can reach the Pi's `/state`
endpoint. It uses only the Python standard library and refreshes the same data
shown by the three.js view.
"""

from __future__ import annotations

import argparse
import http.client
import json
import math
import sys
import time
from urllib.parse import urlsplit


def distance_bar(distance_m: float | None, maximum_m: float, width: int = 32) -> str:
    """Render a fixed-width bar for a distance measured in metres.

    Args:
        distance_m: Current distance in metres, or ``None`` when unavailable.
        maximum_m: Full-scale range in metres, greater than zero.
        width: Number of character cells used for the complete bar.

    Returns:
        An ASCII filled segment followed by a remainder, or dashes when the
        measurement or maximum is invalid.
    """
    if distance_m is None or not math.isfinite(distance_m) or maximum_m <= 0:
        return "-" * width
    filled = max(0, min(width, round(distance_m / maximum_m * width)))
    return "#" * filled + "-" * (width - filled)


def format_pan(pan: object) -> str:
    """Format unitless stereo balance as a left, centre or right percentage.

    Args:
        pan: Normalized balance in -1..1, where -1 is full left and +1 is
            full right.

    Returns:
        The side and percentage, ``Center``, or ``n/a`` when unavailable.
    """
    try:
        value = float(pan)
    except (TypeError, ValueError):
        return "n/a"
    if not math.isfinite(value):
        return "n/a"
    value = max(-1.0, min(1.0, value))
    if abs(value) < 0.005:
        return "Center"
    return f"{'L' if value < 0 else 'R'} {round(abs(value) * 100)}%"


def fetch_snapshot(url: str, timeout_s: float, source_ip: str | None = None) -> tuple[dict, float]:
    """Fetch one JSON sensor snapshot and measure its HTTP round trip.

    Args:
        url: Compute-unit `/state` URL.
        timeout_s: Maximum request wait in seconds.
        source_ip: Optional local interface IPv4 address to bind, such as the
            PC Ethernet adapter address ``192.168.77.1``.

    Returns:
        The decoded snapshot and fetch duration in milliseconds.

    Raises:
        OSError: If the server cannot be reached or returns an invalid body.
        ValueError: If the response is not a JSON object.
    """
    started = time.perf_counter()
    try:
        parsed = urlsplit(url)
        host = parsed.hostname or ""
        connection_type = (http.client.HTTPSConnection if parsed.scheme == "https"
                           else http.client.HTTPConnection)
        connection = connection_type(host, parsed.port, timeout=timeout_s,
                                     source_address=(source_ip, 0) if source_ip else None)
        path = parsed.path or "/"
        if parsed.query:
            path += "?" + parsed.query
        try:
            connection.request("GET", path, headers={"Accept": "application/json"})
            response = connection.getresponse()
            body = response.read()
            if response.status < 200 or response.status >= 300:
                raise OSError(f"HTTP {response.status} {response.reason}")
            state = json.loads(body.decode("utf-8"))
        finally:
            connection.close()
    except (OSError, TimeoutError, http.client.HTTPException) as exc:
        raise OSError(str(exc)) from exc
    elapsed_ms = (time.perf_counter() - started) * 1000.0
    if not isinstance(state, dict):
        raise ValueError("server returned a JSON value that is not an object")
    return state, elapsed_ms


def _format_angles(values: object) -> str:
    """Format a three-axis angle sample in degrees.

    Args:
        values: Three numeric angles in roll, pitch, yaw order, or another value.

    Returns:
        Signed degree values, or a dash when the sample is unavailable.
    """
    if not isinstance(values, list) or len(values) != 3:
        return "n/a"
    try:
        return " / ".join(f"{float(value):+6.1f} deg" for value in values)
    except (TypeError, ValueError):
        return "n/a"


def _pointer_angles(matrix: object) -> str:
    """Convert a row-major pointer rotation into roll, pitch and yaw degrees.

    Args:
        matrix: Nine values mapping pointer body axes into the head frame.

    Returns:
        Signed roll, pitch and yaw values, or a dash for invalid input.
    """
    if not isinstance(matrix, list) or len(matrix) != 9:
        return "n/a"
    try:
        r = [float(value) for value in matrix]
        roll = math.degrees(math.atan2(r[7], r[8]))
        pitch = math.degrees(math.asin(max(-1.0, min(1.0, -r[6]))))
        yaw = math.degrees(math.atan2(r[3], r[0]))
        return f"{roll:+6.1f} deg / {pitch:+6.1f} deg / {yaw:+6.1f} deg"
    except (TypeError, ValueError):
        return "n/a"


def render_snapshot(state: dict, fetch_ms: float, bar_width: int = 32) -> str:
    """Format a sensor snapshot as a readable terminal dashboard.

    Args:
        state: JSON object returned from the compute-unit `/state` endpoint.
        fetch_ms: HTTP fetch duration in milliseconds.
        bar_width: Character-cell width for each range bar.

    Returns:
        A multiline string with IMU, ToF, camera and placement status.
    """
    head = state.get("head") or {}
    pointer = state.get("pointer") or {}
    placement = state.get("placement") or {}
    camera = state.get("camera") or {}
    config = state.get("config") or {}
    maximum = float(config.get("max_range_m") or 4.0)
    tof = pointer.get("range_m")
    hit_distance = placement.get("distance_m")
    camera_distance = camera.get("distance_m")
    camera_age = camera.get("age_s")
    camera_frames = camera.get("frame_count")
    camera_poses = camera.get("detection_count")
    camera_failures = camera.get("read_failures")
    camera_read_ms = camera.get("read_ms")
    camera_detection_ms = camera.get("detection_ms")
    camera_position = camera.get("position_head_m")
    camera_marker_ids = camera.get("marker_ids") or []
    camera_rotation = camera.get("rotation_head")
    camera_orientation_fresh = camera.get("position_fresh") is True
    baseline = placement.get("baseline", "waiting")
    button = pointer.get("button")
    button_text = "pressed" if button is True or button == 1 else "released" if button is False or button == 0 else "n/a"
    gain = placement.get("gain")
    pan = placement.get("pan")
    azimuth = placement.get("azimuth_deg")
    elevation = placement.get("elevation_deg")
    head_health = "fresh" if head.get("fresh") else "stale / unavailable"
    data_mode = "synthetic data" if state.get("mock") is True else "live data"
    head_fresh = head.get("fresh") is True and head.get("euler_deg") is not None
    pointer_present = state.get("pointer") is not None
    if state.get("mock") is True:
        sensor_status = "synthetic sensors"
    elif head_fresh and pointer_present:
        sensor_status = "head + pointer"
    elif pointer_present:
        sensor_status = "pointer only"
    elif head_fresh:
        sensor_status = "head only"
    elif camera.get("position_fresh") is True:
        sensor_status = "vision only"
    elif isinstance(camera_frames, (int, float)) and camera_frames > 0:
        sensor_status = "camera only"
    else:
        sensor_status = "no sensor samples"
    alignment = "aligned" if state.get("aligned") is True else (
        "needs alignment" if state.get("aligned") is False else "unknown")
    camera_source = "tracking" if camera.get("position_fresh") is True else (
        "stale fix" if isinstance(camera_distance, (int, float)) else
        "searching tags" if isinstance(camera_frames, (int, float)) and camera_frames > 0 else "no frames")

    def number(value: object, digits: int = 2) -> str:
        """Format a finite scalar value for the current sensor frame.

        Args:
            value: Numeric measurement; distances use metres and angles use
                degrees according to the caller's field.
            digits: Decimal places to show.

        Returns:
            The formatted value, or a dash when missing or non-finite.
        """
        try:
            converted = float(value)
            return f"{converted:.{digits}f}" if math.isfinite(converted) else "n/a"
        except (TypeError, ValueError):
            return "n/a"

    camera_vector = "n/a"
    if isinstance(camera_position, list) and len(camera_position) == 3:
        camera_vector = ", ".join(number(value, 3) for value in camera_position)
    orientation_matrix = camera_rotation if camera_orientation_fresh else (
        pointer.get("matrix_head") or camera_rotation)
    orientation_source = "ArUco cube" if camera_orientation_fresh else (
        "pointer IMU" if pointer.get("matrix_head") else
        "last camera pose" if camera_rotation else "unavailable")

    return "\n".join((
        "AURIS LIVE SENSOR VIEW  |  Ctrl+C to stop",
        f"Sensors: {sensor_status} | yaw {alignment} | {data_mode} | head {head_health} | camera {camera_source} | fetch {fetch_ms:.1f} ms",
        "",
        f"HEAD r/p/y       {_format_angles(head.get('euler_deg'))}",
        f"RAYGUN r/p/y     {_pointer_angles(orientation_matrix)} | {orientation_source}",
        f"TOF {number(tof, 3)} / {maximum:.1f} m",
        f"  {distance_bar(tof, maximum, bar_width)}",
        f"HIT {number(hit_distance)} / {maximum:.1f} m  gain {number(gain, 3)}  pan {format_pan(pan)}",
        f"  {distance_bar(hit_distance, maximum, bar_width)}",
        f"CAMERA cube XYZ {camera_vector} m | range {number(camera_distance, 3)} m | IDs {','.join(str(value) for value in camera_marker_ids) or 'n/a'} | age {number(camera_age, 3)} s",
        f"CAM frames {number(camera_frames, 0)} | poses {number(camera_poses, 0)} | fails {number(camera_failures, 0)} | read/detect {number(camera_read_ms, 1)}/{number(camera_detection_ms, 1)} ms",
        f"BEAM az/el {number(azimuth, 1)} / {number(elevation, 1)} deg | trigger {button_text}",
    ))


def build_parser() -> argparse.ArgumentParser:
    """Create the command-line options for the terminal sensor view.

    Returns:
        An argument parser with the Pi state URL and polling options.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default="http://192.168.68.59:8081/state",
                        help="Pi /state endpoint (default: Wi-Fi address).")
    parser.add_argument("--interval", type=float, default=0.25,
                        help="Refresh period in seconds (default: 0.25).")
    parser.add_argument("--timeout", type=float, default=2.0,
                        help="HTTP request timeout in seconds (default: 2).")
    parser.add_argument("--source-ip", default=None,
                        help="Bind requests to a local interface IPv4 address, e.g. 192.168.77.1.")
    parser.add_argument("--once", action="store_true",
                        help="Print one snapshot and exit.")
    return parser


def main() -> int:
    """Poll the Pi and refresh the terminal dashboard until interrupted.

    Returns:
        Exit status, zero after a normal stop and one for invalid options.
    """
    args = build_parser().parse_args()
    if args.interval <= 0 or args.timeout <= 0:
        print("interval and timeout must be positive", file=sys.stderr)
        return 1
    try:
        while True:
            try:
                state, fetch_ms = fetch_snapshot(args.url, args.timeout, args.source_ip)
                frame = render_snapshot(state, fetch_ms)
            except (OSError, ValueError, json.JSONDecodeError) as exc:
                frame = f"AURIS SENSOR VIEW | server unavailable\n{args.url}\n{exc}"
            if args.once:
                print(frame)
                return 0
            sys.stdout.write("\x1b[?25l\x1b[2J\x1b[H" + frame + "\n")
            sys.stdout.flush()
            time.sleep(args.interval)
    except KeyboardInterrupt:
        sys.stdout.write("\x1b[?25h\n")
        sys.stdout.flush()
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
