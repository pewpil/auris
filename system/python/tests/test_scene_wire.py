"""Wire-format tests for the frozen SceneState v1 schema (contracts/scene.py)."""

from __future__ import annotations

import struct

import pytest

from auris.contracts.scene import (
    FLAG_BEACON,
    FLAG_HEAD_VALID,
    HEADER_SIZE,
    MAGIC,
    SCHEMA_VERSION,
    SceneObject,
    SceneState,
    SceneStateDecodeError,
    SceneStateError,
)


def canonical_state() -> SceneState:
    """Return the reference SceneState every byte-layout test builds from."""
    return SceneState(
        sequence=7,
        timestamp=0.25,
        head_position_world=(0.0, 0.0, 1.6),
        head_quaternion=(1.0, 0.0, 0.0, 0.0),
        objects=(
            SceneObject(
                object_id=7,
                class_id=1,
                label="bottle",
                azimuth_deg=30.0,
                elevation_deg=-10.0,
                distance_m=2.5,
                position_world=(1.0, -0.5, 0.9),
            ),
        ),
    )


def canonical_bytes() -> bytes:
    """Return the hand-packed v1 bytes `canonical_state()` must encode to.

    Built with plain `struct.pack` from the module docstring's layout tables,
    independent of the implementation under test.
    """
    header = struct.pack(
        "<4sBBId3f4fHH",
        b"AURS",
        1,
        FLAG_HEAD_VALID,
        7,
        0.25,
        0.0,
        0.0,
        1.6,
        1.0,
        0.0,
        0.0,
        0.0,
        1,
        0,
    )
    record = struct.pack("<HBB", 7, 1, 6) + b"bottle"
    tail = struct.pack("<3f3f", 30.0, -10.0, 2.5, 1.0, -0.5, 0.9)
    return header + record + tail


def test_header_size_is_frozen() -> None:
    """The 50-byte header is part of the frozen schema; drift breaks Unity."""
    assert HEADER_SIZE == 50


def test_encode_matches_documented_layout() -> None:
    """`encode` must reproduce the hand-packed canonical bytes exactly."""
    assert canonical_state().encode() == canonical_bytes()


def test_encoded_payload_starts_with_magic_and_version() -> None:
    """The first six bytes must be magic "AURS" + version + HEAD_VALID flag."""
    payload = canonical_state().encode()
    assert payload[:6] == b"AURS" + bytes([SCHEMA_VERSION, FLAG_HEAD_VALID])


def test_object_record_size_is_28_plus_label() -> None:
    """Record size is 28 bytes plus the label — the variable-length part is the label only."""
    state = canonical_state()
    assert len(state.encode()) == HEADER_SIZE + 28 + len("bottle")


def test_decode_roundtrip() -> None:
    """decode(encode(state)) must reconstruct every field of the canonical state."""
    state = canonical_state()
    decoded = SceneState.decode(state.encode())
    assert decoded.sequence == state.sequence
    assert decoded.timestamp == pytest.approx(state.timestamp)
    assert decoded.head_position_world == pytest.approx(state.head_position_world)
    assert decoded.head_quaternion == pytest.approx(state.head_quaternion)
    assert decoded.beacon_object_id == state.beacon_object_id
    assert len(decoded.objects) == len(state.objects)
    sent, got = state.objects[0], decoded.objects[0]
    assert (got.object_id, got.class_id, got.label) == (sent.object_id, sent.class_id, sent.label)
    assert got.azimuth_deg == pytest.approx(sent.azimuth_deg)
    assert got.elevation_deg == pytest.approx(sent.elevation_deg)
    assert got.distance_m == pytest.approx(sent.distance_m)
    assert got.position_world == pytest.approx(sent.position_world)


def test_decode_roundtrip_multiple_objects_and_beacon() -> None:
    """Round-trip must preserve object order and beacon state across several objects."""
    state = SceneState(
        sequence=42,
        timestamp=1.5,
        head_position_world=(0.1, -0.2, 1.7),
        head_quaternion=(0.9238795325112867, 0.0, 0.3826834323650898, 0.0),
        beacon_object_id=3,
        objects=(
            SceneObject(1, 1, "bottle", 10.0, -5.0, 1.5, (1.0, 0.0, 0.5)),
            SceneObject(2, 2, "mug", -120.0, 15.0, 3.25, (-2.0, 1.0, 1.0)),
            SceneObject(3, 3, "book", 0.0, 0.0, 0.5, (0.5, 0.0, 1.5)),
        ),
    )
    decoded = SceneState.decode(state.encode())
    assert decoded.sequence == state.sequence
    assert decoded.timestamp == pytest.approx(state.timestamp)
    assert decoded.head_position_world == pytest.approx(state.head_position_world)
    assert decoded.head_quaternion == pytest.approx(state.head_quaternion)
    assert decoded.beacon_object_id == state.beacon_object_id
    assert [obj.label for obj in decoded.objects] == ["bottle", "mug", "book"]
    for sent, got in zip(state.objects, decoded.objects, strict=True):
        assert got.azimuth_deg == pytest.approx(sent.azimuth_deg)
        assert got.elevation_deg == pytest.approx(sent.elevation_deg)
        assert got.distance_m == pytest.approx(sent.distance_m)
        assert got.position_world == pytest.approx(sent.position_world)


def test_decode_rejects_bad_magic() -> None:
    """A payload whose magic is not "AURS" is not ours and must be rejected."""
    payload = b"XURS" + canonical_bytes()[4:]
    with pytest.raises(SceneStateDecodeError, match="magic"):
        SceneState.decode(payload)


def test_decode_rejects_wrong_version() -> None:
    """Version mismatch means sender and receiver disagree on the schema."""
    payload = canonical_bytes()
    tampered = payload[:4] + bytes([payload[4] + 1]) + payload[5:]
    with pytest.raises(SceneStateDecodeError, match="version"):
        SceneState.decode(tampered)


def test_decode_rejects_truncated_header() -> None:
    """A payload shorter than the header cannot even be parsed structurally."""
    with pytest.raises(SceneStateDecodeError):
        SceneState.decode(canonical_bytes()[:HEADER_SIZE - 1])


def test_decode_rejects_truncated_object_record() -> None:
    """A cut-off object record must be rejected, not silently zero-filled."""
    payload = canonical_bytes()[:-4]
    with pytest.raises(SceneStateDecodeError):
        SceneState.decode(payload)


def test_decode_rejects_trailing_bytes() -> None:
    """Trailing garbage after the last record indicates a corrupted stream."""
    with pytest.raises(SceneStateDecodeError, match="trailing"):
        SceneState.decode(canonical_bytes() + b"\x00")


def test_decode_rejects_oversized_label_length() -> None:
    """A label length beyond the 32-byte cap could desync the parser; reject it."""
    payload = bytearray(canonical_bytes())
    payload[HEADER_SIZE + 3] = 33
    with pytest.raises(SceneStateDecodeError, match="label"):
        SceneState.decode(bytes(payload))


def test_flags_reflect_head_and_beacon() -> None:
    """Absent head/beacon state must encode as clear flags and decode as None."""
    bare = SceneState(sequence=1, timestamp=0.0)
    payload = bare.encode()
    assert payload[5] == 0
    decoded = SceneState.decode(payload)
    assert decoded.head_position_world is None
    assert decoded.head_quaternion is None
    assert decoded.beacon_object_id is None

    beaconed = SceneState(sequence=2, timestamp=0.0, beacon_object_id=9)
    assert SceneState.decode(beaconed.encode()).beacon_object_id == 9
    assert beaconed.encode()[5] & FLAG_BEACON


def test_head_position_and_quaternion_must_be_set_together() -> None:
    """A position without a rotation is a broken pose; the constructor must reject it."""
    with pytest.raises(SceneStateError):
        SceneState(sequence=1, timestamp=0.0, head_position_world=(0.0, 0.0, 1.6))


def test_label_length_is_capped_on_wire() -> None:
    """Labels are capped at 32 UTF-8 bytes; longer ones violate the record layout."""
    with pytest.raises(SceneStateError, match="label"):
        SceneObject(
            object_id=1,
            class_id=0,
            label="x" * 33,
            azimuth_deg=0.0,
            elevation_deg=0.0,
            distance_m=1.0,
            position_world=(0.0, 0.0, 0.0),
        )


def test_utf8_label_counts_bytes_not_characters() -> None:
    """The 32-byte cap is bytes, not characters — multi-byte labels must survive the wire."""
    label = "ñ" * 16
    state = SceneState(
        sequence=1,
        timestamp=0.0,
        objects=(
            SceneObject(1, 0, label, 0.0, 0.0, 1.0, (0.0, 0.0, 0.0)),
        ),
    )
    assert label.encode("utf-8") == b"\xc3\xb1" * 16
    assert SceneState.decode(state.encode()).objects[0].label == label


def test_object_count_is_capped() -> None:
    """u16 count aside, the schema caps objects at 255 — beyond that must be rejected."""
    objects = tuple(
        SceneObject(i, 0, "o", 0.0, 0.0, 1.0, (0.0, 0.0, 0.0))
        for i in range(256)
    )
    with pytest.raises(SceneStateError, match="count"):
        SceneState(sequence=1, timestamp=0.0, objects=objects)


def test_magic_constant() -> None:
    """The wire magic and schema version are frozen contract constants."""
    assert MAGIC == b"AURS"
    assert SCHEMA_VERSION == 1
