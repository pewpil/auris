"""Shared world-frame scene model — latest-wins store for objects and head pose.

The single place where the room state lives: every pose is in the ONE world
frame fixed by calibration. Thread-safe because Prototype capture runs one
thread per camera feeding the localizers.
"""

from __future__ import annotations

import threading

from auris.contracts.head import HeadPose
from auris.contracts.localizer import ObjectObservation


class SceneModel:
    """Latest-wins world-frame scene state, shared across pipeline threads.

    Purpose: decouple the per-camera capture/perception threads from the
    SceneState emitter. Writers replace parts of the scene; the runner reads
    a consistent snapshot under a lock. All stored poses live in the shared
    world frame — no head-relative math happens here (that is
    `auris.pipeline.transform.head_relative`).

    Invariants:
        - The last known head pose is retained across estimator dropouts so
          the soundscape holds steady instead of collapsing.
        - Objects are keyed by `object_id`; a new observation for the same id
          replaces the old one.
    """

    def __init__(self) -> None:
        """Create an empty scene: no objects, no head pose."""
        self._lock = threading.Lock()
        self._objects: dict[int, ObjectObservation] = {}
        self._head: HeadPose | None = None

    def update_objects(self, observations: list[ObjectObservation]) -> None:
        """Replace the object set with the newest observations (full scene per frame).

        Args:
            observations: One complete per-frame object list from a localizer;
                observations with the same `object_id` dedupe (last wins).
        """
        with self._lock:
            self._objects = {obs.object_id: obs for obs in observations}

    def update_head(self, pose: HeadPose) -> None:
        """Record a head pose; the last known pose is retained during dropouts.

        Args:
            pose: Newest 6DoF pose in the world frame; later `head()` calls
                return it until a newer pose replaces it.
        """
        with self._lock:
            self._head = pose

    def head(self) -> HeadPose | None:
        """Return the latest head pose, or `None` before the first estimate."""
        with self._lock:
            return self._head

    def objects(self) -> list[ObjectObservation]:
        """Return the current objects sorted by `object_id` (stable order)."""
        with self._lock:
            return sorted(self._objects.values(), key=lambda obs: obs.object_id)

    def clear(self) -> None:
        """Drop every object and the head pose (used between pipeline sessions)."""
        with self._lock:
            self._objects.clear()
            self._head = None
