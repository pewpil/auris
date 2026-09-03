"""Golden tests for the world → head-relative transform (conventions.md)."""

from __future__ import annotations

import math

import numpy as np
import pytest

from auris.pipeline.transform import (
    head_relative,
    matrix_to_quaternion,
    quaternion_from_axis_angle,
    quaternion_to_matrix,
    rodrigues_to_matrix,
)

IDENTITY = np.eye(3)
ORIGIN = np.zeros(3)


def test_object_straight_ahead() -> None:
    """An object dead ahead of an identity head must read azimuth 0, elevation 0, r = 2 m."""
    azimuth, elevation, distance = head_relative(np.array([2.0, 0.0, 0.0]), ORIGIN, IDENTITY)
    assert azimuth == pytest.approx(0.0)
    assert elevation == pytest.approx(0.0)
    assert distance == pytest.approx(2.0)


def test_object_to_the_right_is_positive_azimuth() -> None:
    """Azimuth sign convention: the user's right is positive (head +y is left)."""
    azimuth, _, _ = head_relative(np.array([0.0, -1.0, 0.0]), ORIGIN, IDENTITY)
    assert azimuth == pytest.approx(90.0)


def test_object_to_the_left_is_negative_azimuth() -> None:
    """Azimuth sign convention: the user's left is negative."""
    azimuth, _, _ = head_relative(np.array([0.0, 1.0, 0.0]), ORIGIN, IDENTITY)
    assert azimuth == pytest.approx(-90.0)


def test_object_above_is_positive_elevation() -> None:
    """Elevation sign convention: above the horizon is positive, up to +90°."""
    _, elevation, distance = head_relative(np.array([0.0, 0.0, 1.0]), ORIGIN, IDENTITY)
    assert elevation == pytest.approx(90.0)
    assert distance == pytest.approx(1.0)


def test_object_below_is_negative_elevation() -> None:
    """Elevation sign convention: below the horizon is negative, down to −90°."""
    _, elevation, _ = head_relative(np.array([0.0, 0.0, -1.0]), ORIGIN, IDENTITY)
    assert elevation == pytest.approx(-90.0)


def test_object_behind_is_180_degrees() -> None:
    """Directly behind resolves to +180°, the inclusive edge of the azimuth range."""
    azimuth, _, distance = head_relative(np.array([-1.0, 0.0, 0.0]), ORIGIN, IDENTITY)
    assert azimuth == pytest.approx(180.0)
    assert distance == pytest.approx(1.0)


def test_coincident_object_has_zero_distance() -> None:
    """A degenerate zero offset must give the benign (0°, 0°, 0 m), not NaN."""
    azimuth, elevation, distance = head_relative(ORIGIN, ORIGIN, IDENTITY)
    assert (azimuth, elevation, distance) == (0.0, 0.0, 0.0)


def test_yaw_left_keeps_world_object_at_zero_azimuth() -> None:
    """World-anchoring: yawing the head left keeps a world-fixed object straight ahead.

    This is the core thesis behavior — the sound field stays glued to the
    world while the head rotates.
    """
    quaternion = quaternion_from_axis_angle(np.array([0.0, 0.0, 1.0]), math.pi / 2)
    rotation = quaternion_to_matrix(quaternion)
    assert np.allclose(rotation @ np.array([1.0, 0.0, 0.0]), [0.0, 1.0, 0.0])
    azimuth, _, distance = head_relative(np.array([0.0, 2.0, 0.0]), ORIGIN, rotation)
    assert azimuth == pytest.approx(0.0, abs=1e-9)
    assert distance == pytest.approx(2.0)


def test_head_position_offsets_distance() -> None:
    """Full 6DoF: translating the head shortens the reported distance."""
    head = np.array([1.0, 2.0, 1.6])
    azimuth, _, distance = head_relative(np.array([2.0, 2.0, 1.6]), head, IDENTITY)
    assert azimuth == pytest.approx(0.0)
    assert distance == pytest.approx(1.0)


@pytest.mark.parametrize(
    ("axis", "angle"),
    [
        (np.array([0.0, 0.0, 1.0]), 0.0),
        (np.array([0.0, 0.0, 1.0]), 0.3),
        (np.array([0.0, 0.0, 1.0]), math.pi / 2),
        (np.array([1.0, 0.0, 0.0]), math.pi),
        (np.array([0.0, 1.0, 0.0]), -2.1),
        (np.array([1.0, 1.0, 1.0]), 1.234),
    ],
)
def test_quaternion_matrix_roundtrip(axis: np.ndarray, angle: float) -> None:
    """Quaternion → matrix → quaternion must recover the same rotation (up to sign)."""
    quaternion = quaternion_from_axis_angle(axis, angle)
    recovered = matrix_to_quaternion(quaternion_to_matrix(quaternion))
    assert abs(float(np.dot(quaternion, recovered))) == pytest.approx(1.0)


def test_rodrigues_matches_axis_angle() -> None:
    """The OpenCV-style rvec conversion must agree with axis-angle for the same rotation."""
    quaternion = quaternion_from_axis_angle(np.array([0.0, 0.0, 1.0]), math.pi / 2)
    expected = quaternion_to_matrix(quaternion)
    assert np.allclose(rodrigues_to_matrix(np.array([0.0, 0.0, math.pi / 2])), expected)


def test_rodrigues_zero_vector_is_identity() -> None:
    """A zero rvec means no rotation; solvePnP can emit this for axis-aligned poses."""
    assert np.allclose(rodrigues_to_matrix(np.zeros(3)), np.eye(3))


def test_rodrigues_produces_rotation_matrix() -> None:
    """The Rodrigues result must be a proper rotation: orthonormal with det +1."""
    rotation = rodrigues_to_matrix(np.array([0.1, -0.4, 0.9]))
    assert np.allclose(rotation @ rotation.T, np.eye(3), atol=1e-12)
    assert pytest.approx(1.0) == float(np.linalg.det(rotation))
