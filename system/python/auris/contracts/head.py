"""`HeadPoseEstimator` contract — full 6DoF head pose in the shared world frame.

Backends: Prototype = ARUCO side-of-head marker pair forming a rigid body
(30–60 Hz); Production = markerless CV first, headset-attached IMU fallback
(orientation only) when the face is turned away or occluded. Smoothing
(One-Euro / low-pass) lives inside the backend so multi-camera handoff never
causes audible jumps.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol, runtime_checkable

import numpy as np

from auris.contracts.frame import Frame


@dataclass(frozen=True, slots=True)
class HeadPose:
    """Full 6DoF head pose in the shared world frame (conventions.md).

    Because the user walks, pose is always position + rotation — never
    rotation alone.

    Attributes:
        position_world: Head origin p ∈ R³, meters, in the world frame
            (right-handed, origin at room floor center, +z up).
        rotation_matrix: R ∈ SO(3) mapping head-frame vectors into the world
            frame: v_world = R · v_head. Columns of R are the head axes —
            +x forward (out of the face), +y left (over the left ear), +z up
            (through the crown) — expressed in world coordinates.
        quaternion: The same world←head rotation as `rotation_matrix`, in
            Hamilton convention (w, x, y, z); must stay consistent with it.
            Wire order is (w, x, y, z); Unity consumers repack.
        timestamp: Monotonic-clock seconds (>= 0) at which the pose held.

    Raises:
        ValueError: At construction (via `__post_init__`) on wrong array
            shapes or a negative timestamp.
    """

    position_world: np.ndarray
    rotation_matrix: np.ndarray
    quaternion: np.ndarray
    timestamp: float

    def __post_init__(self) -> None:
        """Enforce the class invariants; raise ValueError on violation."""
        if self.position_world.shape != (3,):
            raise ValueError("position_world must be a (3,) vector")
        if self.rotation_matrix.shape != (3, 3):
            raise ValueError("rotation_matrix must be 3×3")
        if self.quaternion.shape != (4,):
            raise ValueError("quaternion must be (4,) in (w, x, y, z) order")
        if self.timestamp < 0:
            raise ValueError("timestamp must be non-negative (monotonic seconds)")


@runtime_checkable
class HeadPoseEstimator(Protocol):
    """Estimates head pose from one frame.

    Implementations absorb all sensor-specific work: marker solving or
    markerless CV, multi-camera handoff, and the smoothing that keeps the
    soundscape jitter-free.
    """

    def estimate(self, frame: Frame) -> HeadPose | None:
        """Estimate the head pose from one camera frame.

        Args:
            frame: One frame from any `CameraSource` backend.

        Returns:
            The full 6DoF head pose in the world frame, or `None` when the
            head is not observable in this frame (turned away, occluded). The
            scene model retains the last known pose until a new one arrives.
        """
        ...
