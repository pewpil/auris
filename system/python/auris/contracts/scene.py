"""`SceneState` contract — the frozen UDP wire schema (Python → Unity).

Schema **v1** (draft until the Phase-3 freeze). Hand-rolled fixed layout, all
values little-endian, mirrored byte-for-byte by
`system/unity/Assets/Auris/Scripts/SceneStateDecoder.cs`.

Header (50 bytes)::

    offset  size  field
    0       4     magic b"AURS"
    4       1     schema_version (u8) = 1
    5       1     flags (u8): bit0 HEAD_VALID, bit1 BEACON
    6       4     sequence (u32)
    10      8     timestamp (f64, monotonic seconds)
    18      12    head_position (3×f32, world)
    30      16    head_quaternion (4×f32, w-x-y-z, world←head)
    46      2     object_count (u16)
    48      2     beacon_object_id (u16, 0 when inactive)

Object record (28 + len(label) bytes), repeated object_count times::

    offset  size  field
    0       2     object_id (u16)
    2       1     class_id (u8)
    3       1     label_len (u8, ≤ 32)
    4       n     label (UTF-8)
    4+n     4     azimuth_deg (f32, head-relative, positive right)
    8+n     4     elevation_deg (f32, head-relative, positive up)
    12+n    4     distance_m (f32)
    16+n    12    position_world (3×f32, debug/beacon)

This schema is the swap seam: Prototype freezes it, Production never changes
it, Unity is never touched by the hardware transition.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

MAGIC = b"AURS"
SCHEMA_VERSION = 1

FLAG_HEAD_VALID = 0x01
FLAG_BEACON = 0x02

MAX_LABEL_BYTES = 32
MAX_OBJECTS = 255

_HEADER = struct.Struct("<4sBBId3f4fHH")
_OBJECT_PREFIX = struct.Struct("<HBB")
_OBJECT_TAIL = struct.Struct("<3f3f")

HEADER_SIZE = _HEADER.size
OBJECT_PREFIX_SIZE = _OBJECT_PREFIX.size
OBJECT_TAIL_SIZE = _OBJECT_TAIL.size


class SceneStateError(ValueError):
    """Raised when a SceneState or SceneObject is built violating the wire limits.

    Construction-time validation only: ids outside u16/u8, negative distance,
    oversized labels, head fields half-set, more than `MAX_OBJECTS` objects.
    A SceneState that passes construction always encodes cleanly.
    """


class SceneStateDecodeError(SceneStateError):
    """Raised by `SceneState.decode` for a payload that is not a conforming v1 SceneState.

    Covers bad magic or schema version, truncated headers or object records,
    oversized object counts or label lengths, undecodable UTF-8 labels, and
    trailing bytes. Consumers should treat it as "drop this datagram", not as
    a fatal error.
    """


@dataclass(frozen=True, slots=True)
class SceneObject:
    """One object's head-relative coordinates plus its world-frame position.

    Exactly one variable-length record on the wire (28 bytes + label). The
    head-relative triple is what Unity spatializes; the world position rides
    along for debug rendering and beacon homing.

    Attributes:
        object_id: Stable identity used to track the object across frames and
            to address beacon mode; must fit u16.
        class_id: Numeric detector class index; must fit u8.
        label: Human-readable name rendered by consumers; at most
            `MAX_LABEL_BYTES` UTF-8 bytes (bytes, not characters — see the
            UTF-8 wire test).
        azimuth_deg: Head-relative azimuth in degrees: 0 = straight ahead,
            positive to the user's right, range (-180, 180]. Undefined
            (zero-filled) when the state carries no valid head pose.
        elevation_deg: Head-relative elevation in degrees, positive above the
            horizon, range [-90, +90]; same undefined-as-zero rule.
        distance_m: Head-relative distance in meters (>= 0); same rule.
        position_world: Object origin in the shared world frame, meters —
            valid even without a head pose.

    Raises:
        SceneStateError: At construction (via `__post_init__`) when ids fall
            outside the wire ranges, the distance is negative, or the label
            exceeds the byte cap.
    """

    object_id: int
    class_id: int
    label: str
    azimuth_deg: float
    elevation_deg: float
    distance_m: float
    position_world: tuple[float, float, float]

    def __post_init__(self) -> None:
        """Enforce the wire limits documented on the class."""
        if not 0 <= self.object_id <= 0xFFFF:
            raise SceneStateError(f"object_id out of u16 range: {self.object_id}")
        if not 0 <= self.class_id <= 0xFF:
            raise SceneStateError(f"class_id out of u8 range: {self.class_id}")
        if self.distance_m < 0.0:
            raise SceneStateError("distance_m must be non-negative")
        if len(self.label.encode("utf-8")) > MAX_LABEL_BYTES:
            raise SceneStateError(f"label exceeds {MAX_LABEL_BYTES} UTF-8 bytes")


@dataclass(frozen=True, slots=True)
class SceneState:
    """One update cycle of the soundscape, as rendered by Unity.

    The frozen swap seam: encoded by the Python pipeline, decoded
    byte-for-byte by `system/unity/.../SceneStateDecoder.cs`. Head-relative
    azimuth/elevation/distance are undefined (zero-filled) when no valid head
    pose exists (`FLAG_HEAD_VALID` clear); world positions remain valid. See
    conventions.md for frame and unit definitions.

    Attributes:
        sequence: Per-pipeline monotonic counter starting at 1 (fits u32);
            consumers use it to detect loss and reordering.
        timestamp: Monotonic-clock seconds (f64) of the frame that produced
            this state; one clock domain per host.
        head_position_world: Head origin in meters, world frame, or `None`
            when no valid head pose exists; must be set together with the
            quaternion or not at all.
        head_quaternion: World←head rotation in Hamilton (w, x, y, z) order —
            Unity repacks to UnityEngine.Quaternion(x, y, z, w) — or `None`.
        beacon_object_id: Object currently addressed by beacon mode, or
            `None` when beacon mode is inactive.
        objects: This cycle's objects in detection order; at most
            `MAX_OBJECTS`.

    Raises:
        SceneStateError: At construction (via `__post_init__`) when the head
            fields are half-set, more than `MAX_OBJECTS` objects are given,
            or the beacon id exceeds u16.
    """

    sequence: int
    timestamp: float
    head_position_world: tuple[float, float, float] | None = None
    head_quaternion: tuple[float, float, float, float] | None = None
    beacon_object_id: int | None = None
    objects: tuple[SceneObject, ...] = ()

    def __post_init__(self) -> None:
        """Enforce the pairing and count invariants documented on the class."""
        if (self.head_position_world is None) != (self.head_quaternion is None):
            raise SceneStateError("head position and quaternion must be set together")
        if len(self.objects) > MAX_OBJECTS:
            raise SceneStateError(f"object count exceeds {MAX_OBJECTS}")
        if self.beacon_object_id is not None and not 0 <= self.beacon_object_id <= 0xFFFF:
            raise SceneStateError(f"beacon_object_id out of u16 range: {self.beacon_object_id}")

    def encode(self) -> bytes:
        """Serialize to the frozen v1 wire format.

        Little-endian throughout, laid out exactly as the module docstring
        tables describe: the header carries `FLAG_HEAD_VALID` / `FLAG_BEACON`
        and zero-fills the head fields when they are absent; each object
        becomes one variable-length record (28 bytes + label). `tests/` and
        the Unity golden fixture pin these bytes.

        Returns:
            The exact datagram payload for `SceneStateDecoder.cs`; one
            encoded state travels as one UDP datagram.
        """
        flags = 0
        head_position = self.head_position_world or (0.0, 0.0, 0.0)
        head_quaternion = self.head_quaternion or (1.0, 0.0, 0.0, 0.0)
        if self.head_position_world is not None:
            flags |= FLAG_HEAD_VALID
        beacon_id = 0
        if self.beacon_object_id is not None:
            flags |= FLAG_BEACON
            beacon_id = self.beacon_object_id

        parts: list[bytes] = [
            _HEADER.pack(
                MAGIC,
                SCHEMA_VERSION,
                flags,
                self.sequence,
                self.timestamp,
                *head_position,
                *head_quaternion,
                len(self.objects),
                beacon_id,
            )
        ]
        for obj in self.objects:
            label = obj.label.encode("utf-8")
            parts.append(_OBJECT_PREFIX.pack(obj.object_id, obj.class_id, len(label)))
            parts.append(label)
            parts.append(
                _OBJECT_TAIL.pack(
                    obj.azimuth_deg,
                    obj.elevation_deg,
                    obj.distance_m,
                    *obj.position_world,
                )
            )
        return b"".join(parts)

    @classmethod
    def decode(cls, data: bytes) -> SceneState:
        """Parse a v1 payload, strictly rejecting anything non-conforming.

        The receive-side mirror of `encode`, used by tests and any Python-side
        consumer (logging, replay, latency measurement).

        Args:
            data: Bytes produced by `encode`, possibly arriving over the
                network.

        Returns:
            The decoded state; head fields are `None` when `FLAG_HEAD_VALID`
            was clear and the beacon id is `None` when `FLAG_BEACON` was
            clear.

        Raises:
            SceneStateDecodeError: On bad magic or schema version, a
                truncated header or object record, an oversized object count
                or label length, undecodable UTF-8, or trailing bytes.
        """
        if len(data) < HEADER_SIZE:
            raise SceneStateDecodeError("payload shorter than header")
        (
            magic,
            version,
            flags,
            sequence,
            timestamp,
            hx,
            hy,
            hz,
            qw,
            qx,
            qy,
            qz,
            count,
            beacon_id,
        ) = _HEADER.unpack(data[:HEADER_SIZE])
        if magic != MAGIC:
            raise SceneStateDecodeError(f"bad magic {magic!r}")
        if version != SCHEMA_VERSION:
            raise SceneStateDecodeError(f"unsupported schema version {version}")
        if count > MAX_OBJECTS:
            raise SceneStateDecodeError(f"object count {count} exceeds {MAX_OBJECTS}")

        objects: list[SceneObject] = []
        offset = HEADER_SIZE
        for index in range(count):
            if len(data) - offset < OBJECT_PREFIX_SIZE:
                raise SceneStateDecodeError(f"truncated object record {index}")
            object_id, class_id, label_len = _OBJECT_PREFIX.unpack(
                data[offset : offset + OBJECT_PREFIX_SIZE]
            )
            offset += OBJECT_PREFIX_SIZE
            if label_len > MAX_LABEL_BYTES:
                raise SceneStateDecodeError(f"label length {label_len} exceeds {MAX_LABEL_BYTES}")
            record_size = label_len + OBJECT_TAIL_SIZE
            if len(data) - offset < record_size:
                raise SceneStateDecodeError(f"truncated object record {index}")
            label = data[offset : offset + label_len].decode("utf-8")
            offset += label_len
            azimuth, elevation, distance, wx, wy, wz = _OBJECT_TAIL.unpack(
                data[offset : offset + OBJECT_TAIL_SIZE]
            )
            offset += OBJECT_TAIL_SIZE
            objects.append(
                SceneObject(
                    object_id=object_id,
                    class_id=class_id,
                    label=label,
                    azimuth_deg=azimuth,
                    elevation_deg=elevation,
                    distance_m=distance,
                    position_world=(wx, wy, wz),
                )
            )
        if offset != len(data):
            raise SceneStateDecodeError(f"{len(data) - offset} trailing bytes after last object")

        head_position: tuple[float, float, float] | None = None
        head_quaternion: tuple[float, float, float, float] | None = None
        if flags & FLAG_HEAD_VALID:
            head_position = (hx, hy, hz)
            head_quaternion = (qw, qx, qy, qz)
        return cls(
            sequence=sequence,
            timestamp=timestamp,
            head_position_world=head_position,
            head_quaternion=head_quaternion,
            beacon_object_id=beacon_id if flags & FLAG_BEACON else None,
            objects=tuple(objects),
        )
