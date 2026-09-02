"""Synthetic backends — hardware-free stand-ins for contract validation.

They let the full pipeline (capture → localize → head pose → transform →
SceneState UDP) run end-to-end in tests and demos before any Prototype
hardware exists. They implement the same contracts the real backends will,
so they also serve as reference implementations. Outputs are deterministic
functions of the injected clock.
"""

from auris.synthetic.camera import SyntheticCameraSource
from auris.synthetic.head import SyntheticHeadPoseEstimator
from auris.synthetic.objects import DEFAULT_SCENE_OBJECTS, SyntheticLocalizer, SyntheticObject

__all__ = [
    "DEFAULT_SCENE_OBJECTS",
    "SyntheticCameraSource",
    "SyntheticHeadPoseEstimator",
    "SyntheticLocalizer",
    "SyntheticObject",
]
