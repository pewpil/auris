"""World → head-relative transform — the core spatial math, kept across stages.

Implements `conventions.md` exactly; `tests/test_transform.py` holds the
golden cases. All spatial math lives in Python — Unity only renders.
"""

from __future__ import annotations

import math

import numpy as np


def quaternion_to_matrix(quaternion: np.ndarray) -> np.ndarray:
    """Convert a Hamilton (w, x, y, z) quaternion to a 3×3 rotation matrix.

    Args:
        quaternion: (w, x, y, z) in Hamilton convention, normalized
            internally; represents the world←head rotation per
            conventions.md.

    Returns:
        R ∈ SO(3) with v_world = R @ v_head; the columns of R are the head
        axes (+x forward, +y left, +z up) expressed in world coordinates.
    """
    w, x, y, z = np.asarray(quaternion, dtype=float) / np.linalg.norm(quaternion)
    return np.array(
        [
            [1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - z * w), 2.0 * (x * z + y * w)],
            [2.0 * (x * y + z * w), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - x * w)],
            [2.0 * (x * z - y * w), 2.0 * (y * z + x * w), 1.0 - 2.0 * (x * x + y * y)],
        ]
    )


def matrix_to_quaternion(rotation: np.ndarray) -> np.ndarray:
    """Convert a 3×3 rotation matrix to a Hamilton (w, x, y, z) quaternion.

    Uses Shepperd's method: picks the largest of the trace/diagonal pivots to
    stay numerically stable across the whole rotation range.

    Args:
        rotation: R ∈ SO(3) with v_world = R @ v_head.

    Returns:
        Unit quaternion (w, x, y, z). The sign is arbitrary — q and −q
        encode the same rotation — so callers must compare quaternions by
        |dot|, never component-wise.
    """
    r = np.asarray(rotation, dtype=float)
    trace = float(np.trace(r))
    if trace > 0.0:
        s = math.sqrt(trace + 1.0) * 2.0
        w = 0.25 * s
        x = (r[2, 1] - r[1, 2]) / s
        y = (r[0, 2] - r[2, 0]) / s
        z = (r[1, 0] - r[0, 1]) / s
    elif r[0, 0] > r[1, 1] and r[0, 0] > r[2, 2]:
        s = math.sqrt(1.0 + r[0, 0] - r[1, 1] - r[2, 2]) * 2.0
        w = (r[2, 1] - r[1, 2]) / s
        x = 0.25 * s
        y = (r[0, 1] + r[1, 0]) / s
        z = (r[0, 2] + r[2, 0]) / s
    elif r[1, 1] > r[2, 2]:
        s = math.sqrt(1.0 + r[1, 1] - r[0, 0] - r[2, 2]) * 2.0
        w = (r[0, 2] - r[2, 0]) / s
        x = (r[0, 1] + r[1, 0]) / s
        y = 0.25 * s
        z = (r[1, 2] + r[2, 1]) / s
    else:
        s = math.sqrt(1.0 + r[2, 2] - r[0, 0] - r[1, 1]) * 2.0
        w = (r[1, 0] - r[0, 1]) / s
        x = (r[0, 2] + r[2, 0]) / s
        y = (r[1, 2] + r[2, 1]) / s
        z = 0.25 * s
    q = np.array([w, x, y, z])
    return q / float(np.linalg.norm(q))


def quaternion_from_axis_angle(axis: np.ndarray, angle_rad: float) -> np.ndarray:
    """Build a Hamilton (w, x, y, z) quaternion from an axis-angle rotation.

    Args:
        axis: Rotation axis in R³; normalized internally, must be non-zero.
        angle_rad: Rotation angle about `axis` in radians, right-handed.

    Returns:
        Unit quaternion (w, x, y, z) representing the same rotation.

    Raises:
        ValueError: If `axis` is the zero vector.
    """
    a = np.asarray(axis, dtype=float)
    norm = np.linalg.norm(a)
    if norm == 0.0:
        raise ValueError("axis must be non-zero")
    half = angle_rad / 2.0
    sin_half = math.sin(half)
    return np.array([math.cos(half), *(a / norm * sin_half)])


def rodrigues_to_matrix(rvec: np.ndarray) -> np.ndarray:
    """Convert an OpenCV rotation vector (Rodrigues form) to a 3×3 matrix.

    Phase-1 ARUCO backends need exactly this conversion: `cv2.solvePnP`
    returns a marker's rotation as a 3-vector whose direction is the axis and
    whose norm is the angle in radians.

    Args:
        rvec: OpenCV-style rotation vector (any array shape convertible to
            3 floats).

    Returns:
        R ∈ SO(3); the zero vector yields the identity (no rotation).
    """
    v = np.asarray(rvec, dtype=float).reshape(3)
    theta = float(np.linalg.norm(v))
    if theta == 0.0:
        return np.eye(3)
    k = v / theta
    cross = np.array(
        [
            [0.0, -k[2], k[1]],
            [k[2], 0.0, -k[0]],
            [-k[1], k[0], 0.0],
        ]
    )
    rotation = np.eye(3) + math.sin(theta) * cross + (1.0 - math.cos(theta)) * (cross @ cross)
    return np.asarray(rotation, dtype=float)


def head_relative(
    position_world: np.ndarray,
    head_position_world: np.ndarray,
    rotation_matrix: np.ndarray,
) -> tuple[float, float, float]:
    """Compute one object's head-relative coordinates — the core spatial math.

    This single function is what keeps the soundscape anchored while the user
    turns and walks (conventions.md, README §3.4): the world-frame offset
    d = p_obj − p is rotated into the head frame (+x forward, +y left, +z up)
    by Rᵀ, then reduced to the spherical triple Unity spatializes.

    Args:
        position_world: Object position in meters, shared world frame.
        head_position_world: Head origin in meters, shared world frame — the
            full 6DoF position, never a fixed origin.
        rotation_matrix: World←head rotation R with v_world = R @ v_head.

    Returns:
        (azimuth_deg, elevation_deg, distance_m): azimuth 0 is straight
        ahead, positive to the user's right, in (-180, 180]; elevation is
        positive above the horizon, in [-90, +90]; distance is meters (>= 0).
        Golden cases live in tests/test_transform.py.
    """
    d = np.asarray(position_world, dtype=float) - np.asarray(head_position_world, dtype=float)
    v = np.asarray(rotation_matrix, dtype=float).T @ d
    forward, left, up = (float(component) for component in v)
    azimuth = math.degrees(math.atan2(-left, forward))
    if azimuth <= -180.0:
        azimuth = 180.0
    elevation = math.degrees(math.atan2(up, math.hypot(forward, left)))
    distance = float(np.linalg.norm(d))
    return azimuth, elevation, distance
