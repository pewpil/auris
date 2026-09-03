"""Per-frame update cycle: localize → head pose → transform → sink.

Implements one loop of docs/scenestate-update.md at the 30–60 Hz SceneState
rate. The sink is the Python-side `AudioSink` (the UDP streamer in production
runs; a recorder in tests).
"""

from __future__ import annotations

import threading
import time

from auris.contracts.audio import AudioSink
from auris.contracts.camera import CameraSource
from auris.contracts.frame import Frame
from auris.contracts.head import HeadPoseEstimator
from auris.contracts.localizer import ObjectLocalizer
from auris.contracts.scene import SceneObject, SceneState
from auris.pipeline.scene_model import SceneModel
from auris.pipeline.transform import head_relative


class ScenePipeline:
    """Wires localizer + head estimator + world-frame model into SceneStates.

    Owns one update cycle of docs/scenestate-update.md — pull a frame,
    localize objects, estimate the head, transform every object into
    head-relative coordinates, and hand the SceneState to the sink (the UDP
    streamer in production, a recorder in tests). Backends are injected
    against the contracts, so the hardware swap never touches this class.
    """

    def __init__(
        self,
        localizer: ObjectLocalizer,
        head_estimator: HeadPoseEstimator,
        sink: AudioSink,
        rate_hz: float = 30.0,
    ) -> None:
        """Assemble the pipeline around injected contract backends.

        Args:
            localizer: Object backend producing world-frame observations.
            head_estimator: Head backend producing 6DoF poses, or `None`
                estimates during dropouts.
            sink: SceneState consumer implementing `AudioSink`.
            rate_hz: Target SceneState rate for `run` pacing (30–60 Hz in
                practice).

        Raises:
            ValueError: If `rate_hz` is not positive.
        """
        if rate_hz <= 0.0:
            raise ValueError("rate_hz must be positive")
        self._localizer = localizer
        self._head_estimator = head_estimator
        self._sink = sink
        self._rate_hz = rate_hz
        self._scene = SceneModel()
        self._sequence = 0

    def step(self, frame: Frame) -> SceneState:
        """Run one update cycle over a single frame (docs/scenestate-update.md).

        Sequence: localize → head estimate (latest-wins into the scene model,
        so a `None` estimate keeps the previous pose) → transform every
        observation into head-relative coordinates → emit the numbered
        SceneState to the sink. If no head pose has ever been seen, the
        per-object azimuth/elevation/distance are zero-filled per
        conventions.md.

        Args:
            frame: The frame to process; its timestamp stamps the state.

        Returns:
            The SceneState that was handed to the sink (also useful for tests
            and logging).
        """
        observations = self._localizer.localize(frame)
        pose = self._head_estimator.estimate(frame)
        if pose is not None:
            self._scene.update_head(pose)
        self._scene.update_objects(observations)

        head = self._scene.head()
        scene_objects: list[SceneObject] = []
        for obs in observations:
            if head is not None:
                azimuth, elevation, distance = head_relative(
                    obs.position_world, head.position_world, head.rotation_matrix
                )
            else:
                azimuth = elevation = distance = 0.0
            scene_objects.append(
                SceneObject(
                    object_id=obs.object_id,
                    class_id=obs.class_id,
                    label=obs.label,
                    azimuth_deg=azimuth,
                    elevation_deg=elevation,
                    distance_m=distance,
                    position_world=(
                        float(obs.position_world[0]),
                        float(obs.position_world[1]),
                        float(obs.position_world[2]),
                    ),
                )
            )

        head_position: tuple[float, float, float] | None = None
        head_quaternion: tuple[float, float, float, float] | None = None
        if head is not None:
            head_position = (
                float(head.position_world[0]),
                float(head.position_world[1]),
                float(head.position_world[2]),
            )
            head_quaternion = (
                float(head.quaternion[0]),
                float(head.quaternion[1]),
                float(head.quaternion[2]),
                float(head.quaternion[3]),
            )

        self._sequence += 1
        state = SceneState(
            sequence=self._sequence,
            timestamp=frame.timestamp,
            head_position_world=head_position,
            head_quaternion=head_quaternion,
            objects=tuple(scene_objects),
        )
        self._sink.update_scene(state)
        return state

    def run(
        self,
        source: CameraSource,
        max_steps: int | None = None,
        stop: threading.Event | None = None,
    ) -> int:
        """Paced 30–60 Hz loop pulling frames from `source`; returns steps taken.

        Opens and closes `source` itself (closing happens even on error).
        Pacing is deadline-based against the loop start: slow frames skip
        their sleep instead of drifting, so average throughput tracks
        `rate_hz`.

        Args:
            source: Camera backend to open, read, and close.
            max_steps: Stop after this many frames; `None` runs until the
                source ends (`read` returns `None`) or `stop` fires.
            stop: Optional event (Ctrl+C handler, timer) checked between
                frames for a clean shutdown.

        Returns:
            The number of frames processed — equal to the number of
            SceneStates emitted to the sink.
        """
        source.open()
        interval = 1.0 / self._rate_hz
        start = time.monotonic()
        steps = 0
        try:
            while max_steps is None or steps < max_steps:
                if stop is not None and stop.is_set():
                    break
                frame = source.read()
                if frame is None:
                    break
                self.step(frame)
                steps += 1
                deadline = start + steps * interval
                remaining = deadline - time.monotonic()
                if remaining > 0.0:
                    time.sleep(remaining)
        finally:
            source.close()
        return steps
