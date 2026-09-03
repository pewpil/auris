"""`Frame` contract — the capture abstraction shared by every CameraSource backend.

README §4.1: pixels + timestamp + camera intrinsics. Prototype and Production
capture hardware both produce `Frame`s; nothing downstream knows which hardware
produced them.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np


@dataclass(frozen=True, slots=True)
class CameraIntrinsics:
    """One camera's calibrated pinhole model, OpenCV convention.

    Produced by the Phase-1 chessboard calibration (per-camera intrinsics) and
    consumed by every solvePnP-style backend. The projection maps a camera-
    plane point to pixels as u = fx·X/Z + cx, v = fy·Y/Z + cy.

    Attributes:
        fx: Horizontal focal length, in pixels (must be > 0).
        fy: Vertical focal length, in pixels (must be > 0).
        cx: Principal point x, in pixels from the left image edge.
        cy: Principal point y, in pixels from the top image edge.
        width: Image width in pixels; frames from this camera must match.
        height: Image height in pixels; frames from this camera must match.
        distortion: OpenCV distortion coefficients in OpenCV order
            (k1, k2, p1, p2, k3, ...); an empty tuple means an ideal
            (undistorted) pinhole, as used by synthetic cameras.

    Raises:
        ValueError: At construction (via `__post_init__`) when focal lengths
            or dimensions are non-positive, or a distortion coefficient is
            NaN.
    """

    fx: float
    fy: float
    cx: float
    cy: float
    width: int
    height: int
    distortion: tuple[float, ...] = ()

    def __post_init__(self) -> None:
        """Enforce the class invariants; raise ValueError on violation."""
        if self.fx <= 0 or self.fy <= 0:
            raise ValueError("focal lengths must be positive")
        if self.width <= 0 or self.height <= 0:
            raise ValueError("image dimensions must be positive")
        if any(d != d for d in self.distortion):
            raise ValueError("distortion coefficients must not be NaN")


@dataclass(frozen=True, slots=True)
class Frame:
    """One camera frame flowing through the pipeline.

    The unit of capture handed from any `CameraSource` backend to any
    localizer or head estimator: pixels plus the metadata needed to reason
    about them. Downstream code never knows which hardware produced a frame.

    Attributes:
        image: 3-channel pixel buffer shaped (height, width, 3), uint8, in
            OpenCV convention (OpenCV backends consume it directly).
        timestamp: Capture time in monotonic-clock seconds (>= 0). All
            backends on one host must share a clock domain so frames from
            different cameras are comparable — multi-camera handoff depends
            on it (conventions.md).
        camera_id: Stable identifier of the producing camera (e.g. "cam0"),
            used to route frames to per-camera calibrations.
        frame_index: Increasing per-camera counter for drop/gap detection;
            need not start at 0 after reconnects.
        intrinsics: This camera's calibration when known. solvePnP-style
            backends (ARUCO, YOLO registration) require it; timestamp-only
            consumers may ignore it.

    Raises:
        ValueError: At construction (via `__post_init__`) when the image is
            not H×W×3, the timestamp is negative, or `camera_id` is empty.
    """

    image: np.ndarray
    timestamp: float
    camera_id: str
    frame_index: int = 0
    intrinsics: CameraIntrinsics | None = None

    def __post_init__(self) -> None:
        """Enforce the class invariants; raise ValueError on violation."""
        if self.image.ndim != 3 or self.image.shape[2] != 3:
            raise ValueError("image must be H×W×3")
        if self.timestamp < 0:
            raise ValueError("timestamp must be non-negative (monotonic seconds)")
        if not self.camera_id:
            raise ValueError("camera_id must be non-empty")
