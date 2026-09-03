"""`CameraSource` contract — one streaming camera, Prototype or Production.

Backends: Prototype = phone MJPEG/RTSP over WiFi (RGB-only); Production =
wired USB array, wireless edge nodes, optional depth. A hardware swap means
writing a new implementation of this protocol — never touching pipeline code.
"""

from __future__ import annotations

from typing import Protocol, runtime_checkable

from auris.contracts.frame import CameraIntrinsics, Frame


@runtime_checkable
class CameraSource(Protocol):
    """A streaming camera.

    Lifecycle: `open()` → N × `read()` → `close()`. `read()` returns `None` on
    timeout or end-of-stream instead of blocking forever. Implementations run
    one instance per capture thread; instances must be independent (no shared
    mutable state between sources — mixed Android/iPhone fleets rely on it).
    """

    camera_id: str
    intrinsics: CameraIntrinsics | None

    def open(self) -> None:
        """Acquire the device or stream; must precede the first `read`.

        Raises:
            Backend-specific exception: when the stream cannot be established
                (device busy, URL unreachable, ...). Fail loudly rather than
                degrade silently.
        """
        ...

    def read(self) -> Frame | None:
        """Fetch the next frame from this camera.

        Returns:
            The newest captured frame, or `None` on timeout or end-of-stream
            instead of blocking forever.
        """
        ...

    def close(self) -> None:
        """Release the device or stream.

        Must be safe to call more than once and from cleanup paths (the
        pipeline runner closes in a `finally` block).
        """
        ...
