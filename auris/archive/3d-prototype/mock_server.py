"""Serve the Auris dashboard with mock or direct pointer-USB snapshots.

Run without ``--serial-port`` to exercise HTTP polling with synthetic values.
Pass the pointer's Linux serial device to stream its compact 12-column USB
table into ``/state``. That table supplies pointer orientation and ToF only; it
does not supply the head IMU or camera baseline. ``--delay-ms`` is for mock mode.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import threading
import time
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Sequence
from urllib.parse import urlsplit
from urllib.request import urlopen


class PointerTableReader:
    """Read the pointer ESP32's compact USB sensor table in the background.

    Own the serial descriptor and latest decoded row. The compact firmware
    table runs at 25 Hz and carries pointer orientation, acceleration, range,
    status, age, and flags; it does not contain head-IMU or camera samples.
    """

    def __init__(self, device: str, baud: int) -> None:
        """Open and configure the pointer's USB serial stream.

        Args:
            device: Linux serial path such as ``/dev/ttyUSB0``.
            baud: Serial bit rate matching the pointer USB debug table.

        Raises:
            ImportError: If serial mode is requested on an OS without
                ``termios`` support.
            OSError: If the device cannot be opened or configured.
            ValueError: If the requested baud rate is unavailable.
        """
        import select
        import termios

        speed = getattr(termios, f"B{baud}", None)
        if speed is None:
            raise ValueError(f"unsupported serial baud rate: {baud}")
        self.device = device
        self._select = select.select
        self._termios = termios
        self._fd = os.open(device, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self._latest: dict[str, Any] | None = None
        self._received_at: float | None = None
        try:
            attrs = termios.tcgetattr(self._fd)
            attrs[0] = 0
            attrs[1] = 0
            attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attrs[3] = 0
            attrs[4] = attrs[5] = speed
            termios.tcsetattr(self._fd, termios.TCSANOW, attrs)
            termios.tcflush(self._fd, termios.TCIFLUSH)
        except Exception:
            os.close(self._fd)
            self._fd = -1
            raise
        self._worker = threading.Thread(
            target=self._read_loop, name="auris-pointer-usb", daemon=True)
        self._worker.start()

    def snapshot(self, elapsed_s: float) -> dict[str, Any]:
        """Return the latest live pointer row as a dashboard state snapshot.

        Args:
            elapsed_s: Seconds since this server started.

        Returns:
            JSON-ready pointer state. Head orientation and camera baseline are
            explicitly unavailable because this USB table carries neither.
        """
        with self._lock:
            row = dict(self._latest) if self._latest is not None else None
            received_at = self._received_at
        age_s = (time.monotonic() - received_at
                 if received_at is not None else math.inf)
        if age_s > 0.5:
            row = None
        return build_pointer_snapshot(elapsed_s, row, age_s)

    def close(self) -> None:
        """Stop reading and release the serial device descriptor."""
        self._stop.set()
        if self._fd >= 0:
            os.close(self._fd)
            self._fd = -1
        self._worker.join(timeout=1.0)

    def _read_loop(self) -> None:
        """Decode complete newline-delimited pointer rows until shutdown."""
        pending = bytearray()
        while not self._stop.is_set():
            try:
                ready, _, _ = self._select([self._fd], [], [], 0.2)
                if not ready:
                    continue
                pending.extend(os.read(self._fd, 4096))
            except (OSError, ValueError):
                if self._stop.is_set():
                    return
                time.sleep(0.05)
                continue
            if len(pending) > 8192:
                pending.clear()
            while b"\n" in pending:
                raw, _, remainder = pending.partition(b"\n")
                pending = bytearray(remainder)
                row = parse_pointer_table_row(raw.decode("ascii", "replace"))
                if row is not None:
                    with self._lock:
                        self._latest = row
                        self._received_at = time.monotonic()


class MockStateServer(ThreadingHTTPServer):
    """Serve dashboard files and hold the active snapshot source.

    Own the page directory, mock response delay, and optional pointer USB
    reader for the lifetime of the server. The server is read-only and never
    sends sensor or device-control commands.
    """

    def __init__(self, address: tuple[str, int], page_root: Path,
                 delay_ms: float,
                 pointer_reader: PointerTableReader | None = None) -> None:
        """Bind the dashboard server and store its selected data source.

        Args:
            address: Host and TCP port to bind; port units are integer ports.
            page_root: Directory containing ``index.html`` and the three.js
                assets.
            delay_ms: Artificial wait before each state response, milliseconds.
            pointer_reader: Live USB table reader, or ``None`` for synthetic
                mock snapshots.

        Raises:
            OSError: If the address cannot be bound.
        """
        self.page_root = page_root
        self.delay_ms = delay_ms
        self.started_at = time.monotonic()
        self.pointer_reader = pointer_reader
        super().__init__(address, MockRequestHandler)


class MockRequestHandler(SimpleHTTPRequestHandler):
    """Serve the dashboard and mock or pointer-USB ``/state`` snapshots."""

    def __init__(self, request: Any, client_address: Any,
                 server: MockStateServer) -> None:
        """Serve static files from the root configured by the mock server.

        Args:
            request: Accepted HTTP connection from the standard library server.
            client_address: Client network address supplied by the server.
            server: Server holding the page path and mock response delay.
        """
        super().__init__(request, client_address, server,
                         directory=str(server.page_root))

    def do_GET(self) -> None:
        """Return mock health or sensor state, otherwise serve a page asset.

        ``GET /state`` waits ``server.delay_ms`` milliseconds and returns a
        dashboard-compatible JSON snapshot. It reads synthetic data unless a
        pointer serial reader is configured. Other paths use the static-file
        handler rooted at ``3d/``.
        """
        path = urlsplit(self.path).path
        if path == "/health":
            source = "pointer-usb" if self.server.pointer_reader else "mock"
            self._send_json(200, {"ok": True, "source": source})
            return
        if path == "/state":
            time.sleep(self.server.delay_ms / 1000.0)
            elapsed_s = time.monotonic() - self.server.started_at
            payload = (self.server.pointer_reader.snapshot(elapsed_s)
                       if self.server.pointer_reader
                       else build_snapshot(elapsed_s))
            self._send_json(200, payload)
            return
        super().do_GET()

    def _send_json(self, status: int, payload: dict[str, Any]) -> None:
        """Write one uncached JSON response.

        Args:
            status: HTTP response status code.
            payload: JSON-safe state mapping to send to the browser.
        """
        body = json.dumps(payload, separators=(",", ":"),
                          allow_nan=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)


def _clamp(value: float, low: float, high: float) -> float:
    """Clamp one scalar to an inclusive interval.

    Args:
        value: Scalar without a unit.
        low: Inclusive lower bound in the same unit as ``value``.
        high: Inclusive upper bound in the same unit as ``value``.

    Returns:
        The bounded scalar in the input unit.
    """
    return max(low, min(high, value))


def parse_pointer_table_row(line: str) -> dict[str, Any] | None:
    """Decode one 12-column pointer USB row, ignoring headers and log text.

    Args:
        line: One ASCII line with columns ``t_ms btn roll pitch yaw ax ay az
            range status age flags``. Angles are degrees, acceleration is g,
            range is millimetres, and age is milliseconds.

    Returns:
        A finite row dictionary, or ``None`` for headers, diagnostics, and
        malformed lines.
    """
    tokens = line.split()
    if len(tokens) != 12:
        return None
    try:
        row = {
            "t_ms": int(tokens[0]),
            "button": int(tokens[1]),
            "euler_deg": [float(value) for value in tokens[2:5]],
            "accel_g": [float(value) for value in tokens[5:8]],
            "range_mm": int(tokens[8]),
            "status": int(tokens[9]),
            "age_ms": int(tokens[10]),
            "flags": int(tokens[11], 0),
        }
    except ValueError:
        return None
    numeric = row["euler_deg"] + row["accel_g"]
    if not all(math.isfinite(value) for value in numeric):
        return None
    return row


def rotation_from_euler(euler_deg: Sequence[float]) -> list[float]:
    """Build a row-major pointer rotation from intrinsic ZYX angles.

    Args:
        euler_deg: Pointer roll, pitch, and yaw in degrees, mapping the
            pointer body frame into its gyro-initialised world frame.

    Returns:
        Nine row-major rotation values suitable for ``pointer.matrix_head``.
    """
    roll, pitch, yaw = (math.radians(value) for value in euler_deg)
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)
    return [
        cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr,
        sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr,
        -sp, cp * sr, cp * cr,
    ]


def build_pointer_snapshot(elapsed_s: float, row: dict[str, Any] | None,
                           age_s: float) -> dict[str, Any]:
    """Build a dashboard snapshot from the latest actual pointer USB row.

    Args:
        elapsed_s: Seconds since the telemetry server started.
        row: Decoded 12-column pointer row, or ``None`` when the serial stream
            is missing or stale.
        age_s: Host monotonic age of the received row, seconds.

    Returns:
        JSON-safe snapshot with real pointer readings. Head-IMU and camera
        values remain unavailable; placement is marked as an assumed frame.
    """
    pointer = None
    placement = None
    muted = True
    if row is not None:
        flags = row["flags"]
        matrix = rotation_from_euler(row["euler_deg"])
        beam = [matrix[0], matrix[3], matrix[6]]
        range_valid = (
            bool(flags & 0x04)
            and row["range_mm"] >= 0
            and row["status"] == 0
            and 0 <= row["age_ms"] <= 250
        )
        range_m = row["range_mm"] / 1000.0 if range_valid else None
        pointer = {
            "matrix_head": matrix,
            "range_m": range_m,
            "button": bool(row["button"]),
        }
        if range_m is not None:
            hit = [range_m * value for value in beam]
            distance_m = math.sqrt(sum(value * value for value in hit))
            azimuth = math.degrees(math.atan2(hit[1], hit[0]))
            elevation = math.degrees(math.asin(
                _clamp(hit[2] / distance_m, -1.0, 1.0)))
            gain = _clamp(1.0 - distance_m / 4.0, 0.0, 1.0)
            pan = _clamp(-azimuth / 90.0, -1.0, 1.0)
            placement = {
                "origin_m": [0.0, 0.0, 0.0],
                "beam": beam,
                "hit_m": hit,
                "range_m": range_m,
                "distance_m": distance_m,
                "azimuth_deg": azimuth,
                "elevation_deg": elevation,
                "gain": gain,
                "pan": pan,
                "baseline": "assumed",
                "usable": True,
            }
    return {
        "t": elapsed_s,
        "aligned": False,
        "muted": muted,
        "config": {"max_range_m": 4.0, "full_scale_deg": 90.0},
        "head": {"euler_deg": None, "fresh": False},
        "pointer": pointer,
        "placement": placement,
        "tracked": None,
        "audio": {"gain": 0.0, "pan": placement["pan"] if placement else 0.0},
        "camera": {"distance_m": None, "age_s": None},
        "mock": False,
        "source": "pointer-usb-table",
        "serial_age_s": age_s if row is not None else None,
    }


def build_snapshot(elapsed_s: float) -> dict[str, Any]:
    """Build an animated, dashboard-compatible synthetic sensor snapshot.

    Args:
        elapsed_s: Monotonic seconds since the mock server started.

    Returns:
        JSON-safe state with orientations in degrees, positions and ranges in
        metres, and pointer rotation as a row-major matrix in the head frame.
    """
    roll = math.radians(7.0 * math.sin(elapsed_s * 0.43))
    pitch = math.radians(18.0 * math.sin(elapsed_s * 0.31))
    yaw = math.radians(52.0 * math.sin(elapsed_s * 0.22))
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)

    rotation = [
        cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr,
        sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr,
        -sp, cp * sr, cp * cr,
    ]
    beam = [rotation[0], rotation[3], rotation[6]]
    range_m = 1.25 + 0.55 * math.sin(elapsed_s * 0.91)
    origin = [
        0.32 + 0.06 * math.sin(elapsed_s * 0.37),
        0.12 * math.sin(elapsed_s * 0.29),
        0.14 + 0.03 * math.sin(elapsed_s * 0.51),
    ]
    hit = [origin[i] + range_m * beam[i] for i in range(3)]
    distance_m = math.sqrt(sum(component * component for component in hit))
    azimuth_deg = math.degrees(math.atan2(hit[1], hit[0]))
    elevation_deg = math.degrees(math.asin(_clamp(hit[2] / distance_m, -1, 1)))
    gain = _clamp(1.0 - distance_m / 4.0, 0.0, 1.0)
    pan = _clamp(-azimuth_deg / 90.0, -1.0, 1.0)
    button = (int(elapsed_s) % 6) < 4
    camera_distance_m = math.sqrt(sum(component * component for component in origin))

    return {
        "t": elapsed_s,
        "aligned": True,
        "muted": not button,
        "config": {"max_range_m": 4.0, "full_scale_deg": 90.0},
        "head": {
            "euler_deg": [
                5.0 * math.sin(elapsed_s * 0.19),
                3.0 * math.sin(elapsed_s * 0.27),
                12.0 * math.sin(elapsed_s * 0.13),
            ],
            "fresh": True,
        },
        "pointer": {
            "matrix_head": rotation,
            "range_m": range_m,
            "button": button,
        },
        "placement": {
            "origin_m": origin,
            "beam": beam,
            "hit_m": hit,
            "range_m": range_m,
            "distance_m": distance_m,
            "azimuth_deg": azimuth_deg,
            "elevation_deg": elevation_deg,
            "gain": gain,
            "pan": pan,
            "baseline": "camera",
            "usable": True,
        },
        "tracked": {
            "origin_m": origin,
            "speed_mps": 0.08,
            "holding": False,
            "pointer_stale": False,
            "clamped": False,
        },
        "audio": {"gain": gain if button else 0.0, "pan": pan},
        "camera": {"distance_m": camera_distance_m, "age_s": 0.025},
        "mock": True,
    }


def build_parser() -> argparse.ArgumentParser:
    """Parse dashboard server, mock-delay, and pointer serial options.

    Returns:
        Argument parser with loopback-only binding and optional USB telemetry.
    """
    parser = argparse.ArgumentParser(
        description="Serve the Auris dashboard with mock or pointer USB data.")
    parser.add_argument("--host", default="127.0.0.1",
                        help="Bind address; defaults to loopback only.")
    parser.add_argument("--port", type=int, default=8082,
                        help="HTTP port, default 8082.")
    parser.add_argument("--delay-ms", type=float, default=0.0,
                        help="Artificial /state response delay in milliseconds.")
    parser.add_argument("--serial-port", default=None,
                        help="Read actual pointer table from a Linux serial device.")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Pointer USB table baud rate, default 115200.")
    parser.add_argument("--self-test", action="store_true",
                        help="Measure local HTTP RTTs, then exit.")
    return parser


def run_self_test(page_root: Path, delay_ms: float) -> int:
    """Measure mock-state HTTP round trips from this process.

    Args:
        page_root: Directory containing the dashboard assets.
        delay_ms: Artificial state-response wait, milliseconds.

    Returns:
        Zero when all three HTTP responses contain mock state.

    Raises:
        RuntimeError: If a response is not valid synthetic state.
        OSError: If the loopback server cannot bind or answer a request.
    """
    server = MockStateServer(("127.0.0.1", 0), page_root, delay_ms)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    address, port = server.server_address
    endpoint = f"http://{address}:{port}/state"
    try:
        with urlopen(f"http://{address}:{port}/", timeout=5.0) as response:
            page = response.read().decode("utf-8")
        if "Auris · live system view" not in page:
            raise RuntimeError("mock server did not serve the dashboard page")
        print("dashboard page: HTTP 200")
        for index in range(1, 4):
            started = time.perf_counter()
            with urlopen(endpoint, timeout=5.0) as response:
                payload = json.loads(response.read().decode("utf-8"))
            elapsed_ms = (time.perf_counter() - started) * 1000.0
            if payload.get("mock") is not True or "pointer" not in payload:
                raise RuntimeError("mock /state response is missing sensor data")
            print(f"request {index}: {elapsed_ms:.1f} ms HTTP RTT; "
                  f"configured delay {delay_ms:g} ms")
    finally:
        server.shutdown()
        server.server_close()
        worker.join(timeout=1.0)
    return 0


def main(argv: Sequence[str] | None = None) -> int:
    """Run the mock or pointer telemetry server until interrupted.

    Args:
        argv: Optional command-line arguments, excluding the program name.

    Returns:
        Process status, zero after a normal keyboard interrupt.

    Raises:
        ValueError: If options conflict or the delay is outside 0 to 5000 ms.
        OSError: If the server cannot bind its requested address.
    """
    args = build_parser().parse_args(argv)
    if not 0.0 <= args.delay_ms <= 5000.0:
        raise ValueError("--delay-ms must be between 0 and 5000")
    page_root = Path(__file__).resolve().parent
    if args.self_test and args.serial_port:
        raise ValueError("--self-test cannot be combined with --serial-port")
    if args.self_test:
        return run_self_test(page_root, args.delay_ms)
    pointer_reader = (PointerTableReader(args.serial_port, args.baud)
                      if args.serial_port else None)
    server = MockStateServer((args.host, args.port), page_root, args.delay_ms,
                             pointer_reader)
    source_name = "pointer USB sensor table" if pointer_reader else "mock data"
    print(f"Auris dashboard: http://{args.host}:{args.port}/")
    print(f"/state source: {source_name}; response delay {args.delay_ms:g} ms")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("Stopping mock dashboard.")
    finally:
        server.server_close()
        if pointer_reader:
            pointer_reader.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
