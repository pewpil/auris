"""Synthetic `CameraSource` — blank BGR frames at a fixed rate, clock-injected."""

from __future__ import annotations

import time
from collections.abc import Callable

import numpy as np

from auris.contracts.frame import CameraIntrinsics, Frame


class SyntheticCameraSource:
    """Emits blank frames with monotonic timestamps; no hardware anywhere.

    Purpose: the reference `CameraSource` implementation — it shows backend
    authors what a conforming source looks like and lets the full pipeline
    run end-to-end in tests and Unity bring-up demos before any Prototype
    phone exists. Frames are deterministic functions of the injected clock.

    Attributes:
        camera_id: Identifier stamped on every emitted frame.
        fps: Nominal frame rate in Hz (metadata only — `read` never sleeps).
        size: (width, height) of the emitted blank images in pixels.
        intrinsics: Calibration stamped on frames; defaults to a plausible
            640×480 pinhole (fx = fy = 600, centered principal point).
    """

    def __init__(
        self,
        camera_id: str = "synth-cam",
        fps: float = 30.0,
        size: tuple[int, int] = (640, 480),
        intrinsics: CameraIntrinsics | None = None,
        clock: Callable[[], float] = time.monotonic,
    ) -> None:
        """Configure the synthetic source.

        Args:
            camera_id: Camera identifier stamped on every frame.
            fps: Nominal frame rate in Hz (> 0); only metadata — pacing is
                the runner's job.
            size: (width, height) in pixels of the emitted blank images.
            intrinsics: Optional calibration; a centered default is derived
                from `size` when omitted.
            clock: Monotonic seconds supplier (inject a fake clock in tests);
                defaults to `time.monotonic`.

        Raises:
            ValueError: If `fps` is not positive.
        """
        if fps <= 0.0:
            raise ValueError("fps must be positive")
        self.camera_id = camera_id
        self.fps = fps
        self.size = size
        self.intrinsics: CameraIntrinsics | None = intrinsics or CameraIntrinsics(
            fx=600.0, fy=600.0, cx=size[0] / 2.0, cy=size[1] / 2.0, width=size[0], height=size[1]
        )
        self._clock = clock
        self._t0: float | None = None
        self._index = 0
        self._image = np.zeros((size[1], size[0], 3), dtype=np.uint8)

    def open(self) -> None:
        """Start the clock: timestamps count from this call; the frame index resets."""
        self._t0 = self._clock()
        self._index = 0

    def read(self) -> Frame | None:
        """Return the next blank frame.

        Returns:
            A zeroed H×W×3 uint8 frame stamped with (clock − t0) seconds and
            the running frame index.

        Raises:
            RuntimeError: If called before `open`.
        """
        if self._t0 is None:
            raise RuntimeError("open() must be called before read()")
        timestamp = self._clock() - self._t0
        frame = Frame(
            image=self._image,
            timestamp=timestamp,
            camera_id=self.camera_id,
            frame_index=self._index,
            intrinsics=self.intrinsics,
        )
        self._index += 1
        return frame

    def close(self) -> None:
        """Stop the clock; a later `open` restarts timestamps and the frame index."""
        self._t0 = None
