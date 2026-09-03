"""Prototype `CameraSource` backend: smartphone MJPEG/RTSP over WiFi — Phase 1.

Route map (README §7.1): Android via IP Webcam (native RTSP/MJPEG); iPhone via
DroidCam Linux client or an iOS MJPEG-server app. Lock focus/exposure for
tracking stability. This is the Prototype-only half of the capture contract —
Production swaps in wired USB / wireless edge nodes without touching pipeline
code. Prototype-only assumptions (phone intrinsics, WiFi jitter) stay inside
this module and never leak into Production design.
"""

from __future__ import annotations

from auris.contracts.frame import CameraIntrinsics, Frame


class MjpegCameraSource:
    """One phone camera streaming MJPEG/RTSP, decoded into `Frame`s.

    Purpose: turn spare smartphones into the Prototype camera array. Each
    instance owns one phone's stream (mixed Android/iPhone fleets are fine —
    instances share no mutable state). WiFi latency and phone-specific
    quirks are this module's problem alone; everything downstream sees plain
    `Frame`s.
    """

    def __init__(
        self,
        url: str,
        camera_id: str,
        intrinsics: CameraIntrinsics | None = None,
    ) -> None:
        """Store the stream coordinates and calibration; nothing is opened yet.

        Args:
            url: MJPEG/RTSP URL of the phone stream (Android: IP Webcam;
                iPhone: DroidCam Linux client or an iOS MJPEG-server app).
            camera_id: Stable identifier stamped on every produced frame and
                used by calibration bookkeeping.
            intrinsics: This phone's chessboard calibration, when measured;
                solvePnP-style perception requires it downstream.
        """
        self._url = url
        self.camera_id = camera_id
        self.intrinsics = intrinsics

    def open(self) -> None:
        """Connect to the stream and lock focus/exposure (not yet implemented).

        Locked focus/exposure keeps ARUCO detection stable frame-to-frame.

        Raises:
            NotImplementedError: Until the Phase-1 capture backend lands.
        """
        raise NotImplementedError("Phase 1: MJPEG/RTSP capture backend")

    def read(self) -> Frame | None:
        """Decode the next MJPEG/RTSP frame into a `Frame` (not yet implemented).

        Returns:
            The newest decoded frame stamped with a monotonic timestamp, or
            `None` on timeout or end-of-stream.

        Raises:
            NotImplementedError: Until the Phase-1 capture backend lands.
        """
        raise NotImplementedError("Phase 1: MJPEG/RTSP capture backend")

    def close(self) -> None:
        """Tear down the stream connection (not yet implemented).

        Raises:
            NotImplementedError: Until the Phase-1 capture backend lands.
        """
        raise NotImplementedError("Phase 1: MJPEG/RTSP capture backend")
