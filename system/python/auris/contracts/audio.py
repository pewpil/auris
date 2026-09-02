"""`AudioSink` contract — the rendering side of SceneState.

The Unity build (`system/unity`) is the real implementation: it consumes
SceneStates and renders the binaural soundscape through the HRTF spatializer
onto the output device (Prototype = BT earbuds; Production = wired or
2.4 GHz-dongle headset). On the Python side the same interface is fulfilled by
`auris.transport.streamer.SceneStateStreamer`, which lets the pipeline run
headless in tests and demos.

Beacon mode is carried inside SceneState (`beacon_object_id`, `FLAG_BEACON`);
the Phase-2 interaction layer (keyboard first, then speech) sets it upstream
of the sink.
"""

from __future__ import annotations

from typing import Protocol, runtime_checkable

from auris.contracts.scene import SceneState


@runtime_checkable
class AudioSink(Protocol):
    """Receives one SceneState per update cycle.

    Implementations are the last stage of the pipeline; they must treat each
    state as a self-contained snapshot and never mutate pipeline-side data.
    """

    def update_scene(self, state: SceneState) -> None:
        """Consume one SceneState update cycle.

        Each call supersedes the previous scene (latest-wins). A state with
        `FLAG_HEAD_VALID` clear carries undefined (zero-filled) head-relative
        coordinates: render nothing or hold the last-good state — never reset
        the scene, because world positions remain valid (conventions.md).

        Args:
            state: The newest scene snapshot produced by the pipeline.
        """
        ...
