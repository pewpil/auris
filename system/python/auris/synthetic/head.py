"""Synthetic `HeadPoseEstimator` — fixed head position with a slow yaw sweep.

The sinusoidal yaw makes a Unity demo audibly pan the whole soundscape,
exercising the head-rotation-tracked anchoring without any cameras. Output is
a pure function of the frame timestamp, so it needs no lifecycle and no clock
injection.
"""

from __future__ import annotations

import math

import numpy as np

from auris.contracts.frame import Frame
from auris.contracts.head import HeadPose
from auris.pipeline.transform import quaternion_from_axis_angle, quaternion_to_matrix

WORLD_UP = np.array([0.0, 0.0, 1.0])


class SyntheticHeadPoseEstimator:
    """Head at a fixed position, yawing sinusoidally about world +z.

    Purpose: a hardware-free head that turns — the minimal pose source that
    exercises the full world←head rotation path (the core thesis risk A).
    Yaw 0 faces world +x; the head stays at `position` the whole time.
    """

    def __init__(
        self,
        position: tuple[float, float, float] = (0.0, 0.0, 1.6),
        yaw_amplitude_deg: float = 25.0,
        yaw_period_s: float = 8.0,
    ) -> None:
        """Fix the sweep parameters.

        Args:
            position: Head origin (x, y, z) in meters, world frame; the
                default stands at eye height above the floor-center origin.
            yaw_amplitude_deg: Peak yaw either side of facing world +x, in
                degrees.
            yaw_period_s: Seconds for one full left-right-left sweep.
        """
        self._position = position
        self._yaw_amplitude_deg = yaw_amplitude_deg
        self._yaw_period_s = yaw_period_s

    def estimate(self, frame: Frame) -> HeadPose | None:
        """Compute the head pose for this frame's timestamp.

        Args:
            frame: Provides the timestamp driving the sweep; pixels are
                ignored.

        Returns:
            Head pose at the fixed `position`, rotated by yaw(t) =
            amplitude · sin(2π·(t mod period)/period) about world +z; never
            `None` (this estimator never drops out).
        """
        yaw_rad = math.radians(self._yaw_amplitude_deg) * math.sin(
            2.0 * math.pi * (frame.timestamp % self._yaw_period_s) / self._yaw_period_s
        )
        quaternion = quaternion_from_axis_angle(WORLD_UP, yaw_rad)
        rotation = quaternion_to_matrix(quaternion)
        return HeadPose(
            position_world=np.array(self._position, dtype=float),
            rotation_matrix=rotation,
            quaternion=quaternion,
            timestamp=frame.timestamp,
        )
