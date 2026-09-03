"""`ObjectLocalizer` contract — objects → world-frame positions.

Backends: Prototype = `MarkerLocalizer` (ARUCO solvePnP, RGB-only, markers as
object stand-ins) with early YOLO bring-up; Production = YOLO + one-shot
registration escalating to `TriangulatedLocalizer` (multi-view RGB), depth
camera backend only if both prove insufficient. Tracking/smoothing (Kalman,
ByteTrack) keeps audio positions from flickering between frames.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol, runtime_checkable

import numpy as np

from auris.contracts.frame import Frame


@dataclass(frozen=True, slots=True)
class ObjectObservation:
    """One tracked object's world-frame state at a moment in time.

    Attributes:
        object_id: Stable identity, used to track the object across frames
            and to address beacon mode; must be non-negative and fit u16 on
            the wire.
        label: Human-readable class name (e.g. "bottle"); at most 32 UTF-8
            bytes to fit the SceneState record.
        class_id: Numeric detector class index; must fit u8 on the wire.
        position_world: Object origin in the ONE shared world frame, meters
            (right-handed, origin at room floor center, +z up).
        timestamp: Monotonic-clock seconds (>= 0) at which the observation
            held.
        confidence: Detector/tracker confidence within [0, 1]; marker and
            synthetic backends default to certainty (1.0).

    Raises:
        ValueError: At construction (via `__post_init__`) on a non-(3,)
            position vector, out-of-range confidence, or negative
            `object_id`.
    """

    object_id: int
    label: str
    class_id: int
    position_world: np.ndarray
    timestamp: float
    confidence: float = 1.0

    def __post_init__(self) -> None:
        """Enforce the class invariants; raise ValueError on violation."""
        if self.position_world.shape != (3,):
            raise ValueError("position_world must be a (3,) vector")
        if not 0.0 <= self.confidence <= 1.0:
            raise ValueError("confidence must be within [0, 1]")
        if self.object_id < 0:
            raise ValueError("object_id must be non-negative")


@runtime_checkable
class ObjectLocalizer(Protocol):
    """Produces world-frame object observations from one frame.

    Implementations are per-frame and stateless from the pipeline's point of
    view (any temporal filtering lives inside the backend).
    """

    def localize(self, frame: Frame) -> list[ObjectObservation]:
        """Localize objects in one camera frame.

        Args:
            frame: One frame from any `CameraSource` backend.

        Returns:
            World-frame observations for every object visible in this frame;
            an empty list when nothing is detected. Positions are meters in
            the shared world frame.
        """
        ...
