"""Contract-type validation and protocol conformance of the provided backends."""

from __future__ import annotations

from dataclasses import FrozenInstanceError

import numpy as np
import pytest

from auris.contracts.audio import AudioSink
from auris.contracts.camera import CameraSource
from auris.contracts.frame import CameraIntrinsics, Frame
from auris.contracts.head import HeadPose, HeadPoseEstimator
from auris.contracts.localizer import ObjectLocalizer, ObjectObservation
from auris.contracts.scene import SceneState
from auris.synthetic import (
    SyntheticCameraSource,
    SyntheticHeadPoseEstimator,
    SyntheticLocalizer,
)
from auris.transport.streamer import SceneStateStreamer


def make_intrinsics() -> CameraIntrinsics:
    """Return a plausible 640×480 pinhole calibration for reuse in tests."""
    return CameraIntrinsics(fx=600.0, fy=600.0, cx=320.0, cy=240.0, width=640, height=480)


def make_frame() -> Frame:
    """Return a valid blank frame carrying `make_intrinsics()`."""
    return Frame(
        image=np.zeros((480, 640, 3), dtype=np.uint8),
        timestamp=0.1,
        camera_id="cam0",
        intrinsics=make_intrinsics(),
    )


def test_intrinsics_reject_non_positive_focal_length() -> None:
    """A focal length of zero is not a pinhole camera and must be rejected."""
    with pytest.raises(ValueError, match="focal"):
        CameraIntrinsics(fx=0.0, fy=600.0, cx=320.0, cy=240.0, width=640, height=480)


def test_intrinsics_reject_bad_dimensions() -> None:
    """Zero-sized image dimensions cannot back a real frame and must be rejected."""
    with pytest.raises(ValueError, match="dimensions"):
        CameraIntrinsics(fx=600.0, fy=600.0, cx=320.0, cy=240.0, width=0, height=480)


def test_frame_rejects_non_rgb_image() -> None:
    """Frames are strictly H×W×3; a grayscale buffer has no meaning downstream."""
    with pytest.raises(ValueError, match="H×W×3"):
        Frame(image=np.zeros((480, 640), dtype=np.uint8), timestamp=0.0, camera_id="cam0")


def test_frame_rejects_empty_camera_id() -> None:
    """Every frame must be attributable to a camera; blank ids break that."""
    with pytest.raises(ValueError, match="camera_id"):
        Frame(image=np.zeros((480, 640, 3), dtype=np.uint8), timestamp=0.0, camera_id="")


def test_frame_rejects_negative_timestamp() -> None:
    """Timestamps are monotonic-clock seconds and can never go negative."""
    with pytest.raises(ValueError, match="timestamp"):
        Frame(image=np.zeros((480, 640, 3), dtype=np.uint8), timestamp=-1.0, camera_id="cam0")


def test_contracts_are_frozen() -> None:
    """All contract dataclasses are immutable — backends cannot mutate shared state."""
    frame = make_frame()
    with pytest.raises(FrozenInstanceError):
        setattr(frame, "timestamp", 1.0)  # noqa: B010 — frozen-dataclass runtime check

    observation = ObjectObservation(
        object_id=1,
        label="bottle",
        class_id=1,
        position_world=np.zeros(3),
        timestamp=0.0,
    )
    with pytest.raises(FrozenInstanceError):
        setattr(observation, "label", "mug")  # noqa: B010

    pose = HeadPose(
        position_world=np.zeros(3),
        rotation_matrix=np.eye(3),
        quaternion=np.array([1.0, 0.0, 0.0, 0.0]),
        timestamp=0.0,
    )
    with pytest.raises(FrozenInstanceError):
        setattr(pose, "timestamp", 1.0)  # noqa: B010

    state = SceneState(sequence=1, timestamp=0.0)
    with pytest.raises(FrozenInstanceError):
        setattr(state, "sequence", 2)  # noqa: B010


def test_observation_rejects_bad_position_shape() -> None:
    """Object positions are R³ world-frame points; anything else is a bug upstream."""
    with pytest.raises(ValueError, match="\\(3,\\)"):
        ObjectObservation(
            object_id=1,
            label="bottle",
            class_id=1,
            position_world=np.zeros(2),
            timestamp=0.0,
        )


def test_observation_rejects_out_of_range_confidence() -> None:
    """Confidence is a probability and must stay within [0, 1]."""
    with pytest.raises(ValueError, match="confidence"):
        ObjectObservation(
            object_id=1,
            label="bottle",
            class_id=1,
            position_world=np.zeros(3),
            timestamp=0.0,
            confidence=1.5,
        )


def test_head_pose_rejects_bad_quaternion_shape() -> None:
    """Quaternions are exactly (w, x, y, z); a (3,) array is a caller bug."""
    with pytest.raises(ValueError, match="\\(4,\\)"):
        HeadPose(
            position_world=np.zeros(3),
            rotation_matrix=np.eye(3),
            quaternion=np.array([1.0, 0.0, 0.0]),
            timestamp=0.0,
        )


def test_protocols_are_satisfied_by_backends() -> None:
    """Runtime-checkable protocols must accept every shipped backend — the swap map holds."""
    assert isinstance(SyntheticCameraSource(), CameraSource)
    assert isinstance(SyntheticLocalizer(), ObjectLocalizer)
    assert isinstance(SyntheticHeadPoseEstimator(), HeadPoseEstimator)
    assert isinstance(SceneStateStreamer(), AudioSink)
