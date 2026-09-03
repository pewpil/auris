"""Prototype `HeadPoseEstimator` backend: ARUCO side-of-head marker pair — Phase 1.

Two markers (left/right at ear height) form a rigid body; solving both poses
gives full head position + orientation in the world frame at 30–60 Hz
(README §3.3). Production replaces this with markerless CV plus a
headset-attached IMU fallback — the contract stays.
"""

from __future__ import annotations

from auris.contracts.frame import Frame
from auris.contracts.head import HeadPose


class HeadMarkerPoseEstimator:
    """Estimates 6DoF head pose from a rigid pair of side-of-head markers.

    Purpose: give the soundscape its rotation pivot during Phase 1. The two
    markers (left/right at ear height) form one rigid body; solving their
    poses in the world frame yields both head position and orientation at
    30–60 Hz with ordinary phone cameras. Production swaps in markerless CV
    with a headset-IMU fallback behind the same contract.
    """

    def estimate(self, frame: Frame) -> HeadPose | None:
        """Estimate head pose from the marker pair in one frame (not yet implemented).

        Args:
            frame: One camera frame; `frame.intrinsics` must carry this
                camera's calibration for solvePnP.

        Returns:
            The full 6DoF head pose in the shared world frame (meters +
            rotation), or `None` when neither side marker is visible.

        Raises:
            NotImplementedError: Until the Phase-1 backend lands.
        """
        raise NotImplementedError("Phase 1: ARUCO side-of-head head-pose estimator")
