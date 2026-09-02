"""Auris data contracts — defined before implementation, shared by both stages.

Contract-first (README §4): `Frame`, `CameraSource`, `ObjectLocalizer`,
`HeadPoseEstimator`, `SceneState`, `AudioSink`. Prototype code is built
against these contracts, not against its hardware; the Production hardware
swap touches only backend implementations. Coordinate and unit rules live in
`conventions.md` next to this module.
"""

from auris.contracts.audio import AudioSink
from auris.contracts.camera import CameraSource
from auris.contracts.frame import CameraIntrinsics, Frame
from auris.contracts.head import HeadPose, HeadPoseEstimator
from auris.contracts.localizer import ObjectLocalizer, ObjectObservation
from auris.contracts.scene import (
    FLAG_BEACON,
    FLAG_HEAD_VALID,
    HEADER_SIZE,
    MAX_LABEL_BYTES,
    MAX_OBJECTS,
    SCHEMA_VERSION,
    SceneObject,
    SceneState,
    SceneStateDecodeError,
    SceneStateError,
)

__all__ = [
    "FLAG_BEACON",
    "FLAG_HEAD_VALID",
    "HEADER_SIZE",
    "MAX_LABEL_BYTES",
    "MAX_OBJECTS",
    "SCHEMA_VERSION",
    "AudioSink",
    "CameraIntrinsics",
    "CameraSource",
    "Frame",
    "HeadPose",
    "HeadPoseEstimator",
    "ObjectLocalizer",
    "ObjectObservation",
    "SceneObject",
    "SceneState",
    "SceneStateDecodeError",
    "SceneStateError",
]
