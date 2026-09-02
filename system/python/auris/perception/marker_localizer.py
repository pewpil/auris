"""Prototype `ObjectLocalizer` backend: ARUCO solvePnP on tagged objects — Phase 1.

A marker's solvePnP pose gives its 3D position directly in the camera/world
frame — no plane assumption, any height (README §3.2). RGB-only in the
Prototype: no triangulation, no depth. OpenCV rvec → rotation matrix goes
through `auris.pipeline.transform.rodrigues_to_matrix`.
"""

from __future__ import annotations

from auris.contracts.frame import Frame
from auris.contracts.localizer import ObjectObservation


class MarkerLocalizer:
    """Localizes ARUCO-tagged objects into world-frame observations.

    Purpose: stand in for real object detection during Phase 1. Every physical
    object carries an ARUCO marker; solving the marker pose with the frame's
    camera intrinsics yields the object's 3D position directly in the shared
    world frame — no plane assumption, any height. Production swaps this
    backend for YOLO + registration / triangulation behind the same contract.
    """

    def localize(self, frame: Frame) -> list[ObjectObservation]:
        """Detect ARUCO-tagged objects in one frame (not yet implemented).

        Args:
            frame: One camera frame; `frame.intrinsics` must carry this
                camera's calibration for solvePnP.

        Returns:
            One world-frame observation per recognized marker-tagged object
            (meters, shared world frame), or an empty list when none are
            visible.

        Raises:
            NotImplementedError: Until the Phase-1 backend lands.
        """
        raise NotImplementedError("Phase 1: ARUCO solvePnP object localizer")
