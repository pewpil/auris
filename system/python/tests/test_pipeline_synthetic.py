"""End-to-end pipeline tests on synthetic backends, including the UDP path."""

from __future__ import annotations

import math
import socket
from collections.abc import Callable

import numpy as np
import pytest

from auris.contracts.frame import Frame
from auris.contracts.head import HeadPose, HeadPoseEstimator
from auris.contracts.scene import SceneState
from auris.pipeline.runner import ScenePipeline
from auris.pipeline.transform import quaternion_from_axis_angle
from auris.synthetic import (
    DEFAULT_SCENE_OBJECTS,
    SyntheticCameraSource,
    SyntheticHeadPoseEstimator,
    SyntheticLocalizer,
)
from auris.transport.streamer import SceneStateStreamer


class FakeClock:
    """Injectable deterministic clock: returns a value the test advances directly."""

    def __init__(self) -> None:
        """Start the fake clock at t = 0."""
        self.now = 0.0

    def __call__(self) -> float:
        """Return the current fake time in monotonic-style seconds."""
        return self.now


class RecordingSink:
    """`AudioSink` test double that records every SceneState it receives."""

    def __init__(self) -> None:
        """Start with an empty recording."""
        self.states: list[SceneState] = []

    def update_scene(self, state: SceneState) -> None:
        """Append the received state; never drops or reorders.

        Args:
            state: The pipeline's newest scene snapshot.
        """
        self.states.append(state)


class NullHeadEstimator:
    """`HeadPoseEstimator` stub that never sees the head — simulates dropout."""

    def estimate(self, frame: Frame) -> HeadPose | None:
        """Report the head as unobservable in every frame.

        Args:
            frame: Ignored.

        Returns:
            Always `None`, forcing the scene model to retain its last pose.
        """
        return None


def make_pipeline(
    sink: RecordingSink,
    clock: FakeClock | None = None,
    head_estimator: HeadPoseEstimator | None = None,
) -> tuple[ScenePipeline, SyntheticCameraSource, FakeClock]:
    """Assemble a synthetic pipeline wired to a recording sink.

    Args:
        sink: Records every SceneState the pipeline emits.
        clock: Fake clock for the camera source; a fresh one is created when
            omitted.
        head_estimator: Override the head backend (e.g. `NullHeadEstimator`);
            defaults to the yawing synthetic estimator.

    Returns:
        (pipeline, source, clock) — the source is returned open-able and the
        clock so tests can advance time.
    """
    active_clock = clock or FakeClock()
    source = SyntheticCameraSource(fps=30.0, clock=active_clock)
    estimator = (
        head_estimator
        if head_estimator is not None
        else SyntheticHeadPoseEstimator()
    )
    pipeline = ScenePipeline(
        localizer=SyntheticLocalizer(DEFAULT_SCENE_OBJECTS),
        head_estimator=estimator,
        sink=sink,
        rate_hz=30.0,
    )
    return pipeline, source, active_clock


def test_run_produces_sequential_scene_states() -> None:
    """The paced loop must emit one SceneState per frame with sequence 1..N."""
    sink = RecordingSink()
    pipeline, source, _ = make_pipeline(sink)
    steps = pipeline.run(source, max_steps=5)
    assert steps == 5
    assert [state.sequence for state in sink.states] == [1, 2, 3, 4, 5]


def test_first_state_carries_expected_head_relative_goldens() -> None:
    """First frame's snapshot must match hand-computed azimuth/elevation/distance goldens."""
    sink = RecordingSink()
    pipeline, source, _ = make_pipeline(sink)
    source.open()
    frame = source.read()
    assert frame is not None
    state = pipeline.step(frame)

    assert state.sequence == 1
    assert state.head_position_world == pytest.approx((0.0, 0.0, 1.6))
    assert state.head_quaternion == pytest.approx((1.0, 0.0, 0.0, 0.0))

    bottle = state.objects[0]
    assert bottle.label == "bottle"
    orbit_offset = 0.30
    assert bottle.position_world == pytest.approx((1.5 + orbit_offset, -0.5, 0.10))
    assert bottle.azimuth_deg == pytest.approx(math.degrees(math.atan2(0.5, 1.5 + orbit_offset)))
    expected_elevation = math.degrees(math.atan2(-1.5, math.hypot(1.5 + orbit_offset, 0.5)))
    assert bottle.elevation_deg == pytest.approx(expected_elevation)
    assert bottle.distance_m == pytest.approx(math.sqrt(5.74))

    mug = state.objects[1]
    assert mug.label == "mug"
    assert mug.azimuth_deg == pytest.approx(math.degrees(math.atan2(-1.2, -1.0)))

    book = state.objects[2]
    assert book.label == "book"
    assert book.elevation_deg == pytest.approx(
        math.degrees(math.atan2(1.10 - 1.6, math.hypot(0.8, 1.5)))
    )


def test_head_yaw_at_quarter_period_is_encoded() -> None:
    """A yawed frame must carry the exact expected rotation quaternion on the state."""
    sink = RecordingSink()
    pipeline, source, _ = make_pipeline(sink)
    source.open()
    frame = source.read()
    assert frame is not None
    yawed = Frame(
        image=frame.image,
        timestamp=2.0,
        camera_id=frame.camera_id,
        frame_index=frame.frame_index,
        intrinsics=frame.intrinsics,
    )
    state = pipeline.step(yawed)
    assert state.head_quaternion is not None
    expected = quaternion_from_axis_angle(np.array([0.0, 0.0, 1.0]), math.radians(25.0))
    assert state.head_quaternion == pytest.approx(tuple(expected))


def test_head_pose_is_retained_during_estimator_dropout() -> None:
    """A `None` estimate must not clear the scene: the last pose keeps anchoring audio."""
    sink = RecordingSink()
    pipeline, source, _ = make_pipeline(sink)
    source.open()
    first_frame = source.read()
    assert first_frame is not None
    first = pipeline.step(first_frame)
    assert first.head_position_world is not None

    pipeline._head_estimator = NullHeadEstimator()
    second_frame = source.read()
    assert second_frame is not None
    second = pipeline.step(second_frame)
    assert second.head_position_world == first.head_position_world
    assert second.sequence == 2


def test_scene_state_reaches_udp_socket_undamaged() -> None:
    """The real UDP path must deliver a byte-identical-decodable SceneState."""
    receiver = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    receiver.bind(("127.0.0.1", 0))
    receiver.settimeout(5.0)
    host, port = receiver.getsockname()

    sink = RecordingSink()
    _, source, _ = make_pipeline(sink)
    source.open()
    streamer = SceneStateStreamer(host=host, port=port)
    wire_pipeline = ScenePipeline(
        localizer=SyntheticLocalizer(DEFAULT_SCENE_OBJECTS),
        head_estimator=SyntheticHeadPoseEstimator(),
        sink=streamer,
        rate_hz=30.0,
    )
    try:
        wire_frame = source.read()
        assert wire_frame is not None
        expected = wire_pipeline.step(wire_frame)
        payload, _ = receiver.recvfrom(4096)
    finally:
        streamer.close()
        receiver.close()

    received = SceneState.decode(payload)
    assert received.sequence == expected.sequence
    assert received.timestamp == pytest.approx(expected.timestamp)
    assert received.head_position_world == pytest.approx(expected.head_position_world)
    assert len(received.objects) == len(expected.objects)
    assert received.objects[0].label == "bottle"
    assert received.objects[0].azimuth_deg == pytest.approx(expected.objects[0].azimuth_deg)
    assert streamer.packets_sent == 1


def test_clock_type_alias_is_callable() -> None:
    """The injected clock must satisfy the plain `Callable[[], float]` contract."""
    clock: Callable[[], float] = FakeClock()
    assert clock() == 0.0
