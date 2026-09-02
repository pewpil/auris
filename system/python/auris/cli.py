"""`auris` command-line entry points.

`auris stream --synthetic` runs the full pipeline on synthetic backends and
streams SceneStates over UDP — usable as a Unity bring-up signal before any
Prototype hardware exists.
"""

from __future__ import annotations

import argparse
import signal
import sys
import threading

from auris import __version__
from auris.pipeline.runner import ScenePipeline
from auris.synthetic import (
    DEFAULT_SCENE_OBJECTS,
    SyntheticCameraSource,
    SyntheticHeadPoseEstimator,
    SyntheticLocalizer,
)
from auris.transport.streamer import DEFAULT_HOST, DEFAULT_PORT, SceneStateStreamer


def _build_parser() -> argparse.ArgumentParser:
    """Build the argument parser for all `auris` subcommands.

    Returns:
        Parser with `--version` and the `stream` / `calibrate` subcommands;
        each subcommand stores its handler in `args.func` for `main` to
        dispatch.
    """
    parser = argparse.ArgumentParser(
        prog="auris",
        description="Auris prototype pipeline (contract-first skeleton).",
    )
    parser.add_argument("--version", action="version", version=f"auris {__version__}")
    subparsers = parser.add_subparsers(dest="command", required=True)

    stream = subparsers.add_parser(
        "stream",
        help="run the SceneState pipeline and stream over UDP",
    )
    stream.add_argument("--host", default=DEFAULT_HOST, help="UDP destination host")
    stream.add_argument("--port", type=int, default=DEFAULT_PORT, help="UDP destination port")
    stream.add_argument("--rate", type=float, default=30.0, help="SceneState rate in Hz")
    stream.add_argument(
        "--synthetic",
        action="store_true",
        help="run with synthetic backends (no hardware; Phase 1 adds real capture)",
    )
    stream.add_argument(
        "--seconds",
        type=float,
        default=None,
        help="stop after N seconds (default: run until interrupted)",
    )
    stream.set_defaults(func=_run_stream)

    calibrate = subparsers.add_parser(
        "calibrate",
        help="camera calibration tooling (Phase 1: intrinsics + multi-camera extrinsics)",
    )
    calibrate.set_defaults(func=_run_calibrate)
    return parser


def _run_stream(args: argparse.Namespace) -> int:
    """Execute `auris stream`: run the pipeline and stream SceneStates over UDP.

    Currently requires --synthetic; hardware capture backends arrive in
    Phase 1. Installs SIGINT/SIGTERM handlers and an optional timer so both
    Ctrl+C and `--seconds` stop the loop cleanly, and restores the previous
    handlers afterwards.

    Args:
        args: Parsed `stream` subcommand arguments (host, port, rate,
            synthetic, seconds).

    Returns:
        Process exit code: 0 after a clean stop, 2 when hardware mode is
        requested before it exists.
    """
    if not args.synthetic:
        print(
            "error: only --synthetic is available; hardware capture backends "
            "arrive in Phase 1 (phone MJPEG/RTSP, ARUCO localizers).",
            file=sys.stderr,
        )
        return 2

    source = SyntheticCameraSource(fps=args.rate)
    pipeline = ScenePipeline(
        localizer=SyntheticLocalizer(DEFAULT_SCENE_OBJECTS),
        head_estimator=SyntheticHeadPoseEstimator(),
        sink=SceneStateStreamer(host=args.host, port=args.port),
        rate_hz=args.rate,
    )

    stop = threading.Event()

    def _interrupt(_signum: int, _frame: object) -> None:
        """Signal handler: request the stream loop to stop at the next frame boundary."""
        stop.set()

    timer: threading.Timer | None = None
    if args.seconds is not None:
        timer = threading.Timer(args.seconds, stop.set)
        timer.start()

    previous_int = signal.signal(signal.SIGINT, _interrupt)
    previous_term = signal.signal(signal.SIGTERM, _interrupt)
    print(
        f"streaming SceneState v1 -> udp://{args.host}:{args.port} "
        f"at {args.rate:g} Hz ({len(DEFAULT_SCENE_OBJECTS)} synthetic objects); "
        "Ctrl+C to stop"
    )
    try:
        steps = pipeline.run(source, stop=stop)
    finally:
        if timer is not None:
            timer.cancel()
        signal.signal(signal.SIGINT, previous_int)
        signal.signal(signal.SIGTERM, previous_term)
        source.close()
    print(f"sent {steps} SceneStates")
    return 0


def _run_calibrate(_args: argparse.Namespace) -> int:
    """Placeholder for Phase-1 calibration tooling (intrinsics + extrinsics).

    Returns:
        Process exit code 2 until implemented.
    """
    print(
        "calibrate: not implemented yet — Phase 1 delivers chessboard intrinsics "
        "and multi-camera extrinsic calibration into the shared world frame.",
        file=sys.stderr,
    )
    return 2


def main(argv: list[str] | None = None) -> int:
    """CLI entry point (`python -m auris` and the `auris` console script).

    Args:
        argv: Raw argument list; `None` means `sys.argv[1:]`.

    Returns:
        Process exit code from the selected subcommand handler.
    """
    parser = _build_parser()
    args = parser.parse_args(argv)
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
