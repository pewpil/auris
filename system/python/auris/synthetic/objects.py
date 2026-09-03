"""Synthetic `ObjectLocalizer` — deterministic objects orbiting fixed bases.

Objects sit at varied heights (floor, shelf, furniture) per the concept;
each may orbit its base position slowly in the floor plane so a Unity demo
audibly shows movement.
"""

from __future__ import annotations

import math
from collections.abc import Sequence
from dataclasses import dataclass

import numpy as np

from auris.contracts.frame import Frame
from auris.contracts.localizer import ObjectObservation

CLASS_UNKNOWN = 0
CLASS_BOTTLE = 1
CLASS_MUG = 2
CLASS_BOOK = 3


@dataclass(frozen=True, slots=True)
class SyntheticObject:
    """One synthetic object's identity and motion model.

    Attributes:
        object_id: Stable identity surfaced on the wire (u16) and used by
            beacon mode.
        label: Human-readable name (e.g. "bottle").
        class_id: Detector class index (see the CLASS_* constants above).
        base_position: (x, y, z) of the orbit center in meters, shared world
            frame — varied heights put objects on the floor, shelves, and
            furniture per the concept.
        orbit_radius_m: Floor-plane orbit radius in meters; 0 pins the
            object to its base.
        orbit_period_s: Seconds per revolution; ignored when the radius is 0.
    """

    object_id: int
    label: str
    class_id: int
    base_position: tuple[float, float, float]
    orbit_radius_m: float = 0.0
    orbit_period_s: float = 12.0


# Demo scene: a bottle orbiting on the floor, a mug on a shelf, a book up high.
DEFAULT_SCENE_OBJECTS: tuple[SyntheticObject, ...] = (
    SyntheticObject(
        object_id=1,
        label="bottle",
        class_id=CLASS_BOTTLE,
        base_position=(1.5, -0.5, 0.10),
        orbit_radius_m=0.30,
    ),
    SyntheticObject(
        object_id=2,
        label="mug",
        class_id=CLASS_MUG,
        base_position=(-1.0, 1.2, 0.75),
    ),
    SyntheticObject(
        object_id=3,
        label="book",
        class_id=CLASS_BOOK,
        base_position=(0.8, 1.5, 1.10),
    ),
)


class SyntheticLocalizer:
    """Returns each object's world position (base + slow orbit) for a timestamp.

    Purpose: the reference `ObjectLocalizer` — fully deterministic from the
    frame timestamp, so golden tests can predict every position exactly and
    Unity demos can show objects that move.
    """

    def __init__(self, objects: Sequence[SyntheticObject] = DEFAULT_SCENE_OBJECTS) -> None:
        """Fix the object set to emit on every frame.

        Args:
            objects: Scene definition; defaults to the three-object demo
                scene above.
        """
        self._objects = tuple(objects)

    def localize(self, frame: Frame) -> list[ObjectObservation]:
        """Emit world-frame observations for every configured object.

        Args:
            frame: Provides the timestamp driving the orbits; pixels are
                ignored.

        Returns:
            One observation per configured object, in configuration order;
            each position is base_position + (radius·cos θ, radius·sin θ, 0)
            where θ advances one revolution per `orbit_period_s`. All
            positions are meters in the shared world frame.
        """
        observations: list[ObjectObservation] = []
        for obj in self._objects:
            angle = 0.0
            if obj.orbit_radius_m > 0.0 and obj.orbit_period_s > 0.0:
                angle = 2.0 * math.pi * (frame.timestamp % obj.orbit_period_s) / obj.orbit_period_s
            position = np.array(
                [
                    obj.base_position[0] + obj.orbit_radius_m * math.cos(angle),
                    obj.base_position[1] + obj.orbit_radius_m * math.sin(angle),
                    obj.base_position[2],
                ],
                dtype=float,
            )
            observations.append(
                ObjectObservation(
                    object_id=obj.object_id,
                    label=obj.label,
                    class_id=obj.class_id,
                    position_world=position,
                    timestamp=frame.timestamp,
                )
            )
        return observations
