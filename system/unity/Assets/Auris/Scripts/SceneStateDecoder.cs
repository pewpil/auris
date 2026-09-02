using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEngine;

namespace Auris
{
    /// <summary>
    /// One tracked object inside a scene snapshot, already converted to
    /// Unity-friendly types. Mirrors one variable-length wire record
    /// (28 bytes + label) of the frozen SceneState v1 schema.
    /// </summary>
    [Serializable]
    public sealed class SceneObjectSnapshot
    {
        /// <summary>Stable identity; beacon mode addresses objects by it.</summary>
        public ushort ObjectId;

        /// <summary>Numeric detector class index.</summary>
        public byte ClassId;

        /// <summary>Human-readable name (UTF-8 decoded, ≤ 32 bytes on the wire).</summary>
        public string Label;

        /// <summary>Head-relative azimuth in degrees: 0 ahead, positive right; undefined (0) when <see cref="SceneSnapshot.HeadValid"/> is false.</summary>
        public float AzimuthDeg;

        /// <summary>Head-relative elevation in degrees, positive above the horizon; same undefined rule.</summary>
        public float ElevationDeg;

        /// <summary>Head-relative distance in meters (≥ 0); same undefined rule.</summary>
        public float DistanceM;

        /// <summary>Object origin in the shared world frame, meters — valid even without a head pose.</summary>
        public Vector3 WorldPosition;
    }

    /// <summary>
    /// One update cycle of the soundscape, as rendered by Unity — the decoded
    /// form of a SceneState v1 datagram. Consumers treat snapshots as
    /// latest-wins: each supersedes the previous one entirely.
    /// </summary>
    [Serializable]
    public sealed class SceneSnapshot
    {
        /// <summary>Per-pipeline monotonic counter starting at 1, for loss/reorder detection.</summary>
        public uint Sequence;

        /// <summary>Monotonic-clock seconds of the frame that produced this state (latency instrumentation reads this).</summary>
        public double Timestamp;

        /// <summary>False when no valid head pose exists: head-relative fields are zero-filled; hold last-good or render nothing.</summary>
        public bool HeadValid;

        /// <summary>Head origin in meters, world frame (zeroed when <see cref="HeadValid"/> is false).</summary>
        public Vector3 HeadPosition;

        /// <summary>World←head rotation, repacked from wire (w, x, y, z) into Unity's (x, y, z, w).</summary>
        public Quaternion HeadRotation;

        /// <summary>True when beacon mode is active for <see cref="BeaconObjectId"/>.</summary>
        public bool BeaconActive;

        /// <summary>Object id currently addressed by beacon mode (0 when inactive).</summary>
        public ushort BeaconObjectId;

        /// <summary>This cycle's objects, in detection order (at most 255).</summary>
        public List<SceneObjectSnapshot> Objects;
    }

    /// <summary>
    /// Strict decoder for the frozen SceneState v1 wire schema — the C# mirror
    /// of <c>auris.contracts.scene</c> (50-byte header + 28+label bytes per
    /// object, all little-endian, magic "AURS"). Accepts exactly what the
    /// Python encoder emits and rejects everything else; the 84-byte golden
    /// fixture in <c>unity/README.md</c> pins the layout on both sides. The
    /// schema freezes in Phase 3 — any change requires a version bump on
    /// both sides.
    /// </summary>
    public static class SceneStateDecoder
    {
        /// <summary>Only schema version 1 is accepted.</summary>
        public const byte SchemaVersion = 1;

        /// <summary>Header size in bytes; must match Python's HEADER_SIZE.</summary>
        public const int HeaderSize = 50;

        /// <summary>Maximum UTF-8 label length per object record; must match Python.</summary>
        public const int MaxLabelBytes = 32;

        /// <summary>Maximum objects per datagram; must match Python.</summary>
        public const int MaxObjects = 255;

        /// <summary>Header flag bit 0: head position/quaternion are valid.</summary>
        private const byte FlagHeadValid = 0x01;

        /// <summary>Header flag bit 1: beacon mode is active.</summary>
        private const byte FlagBeacon = 0x02;

        /// <summary>Wire magic "AURS" as bytes.</summary>
        private static readonly byte[] Magic = { 0x41, 0x55, 0x52, 0x53 };

        /// <summary>
        /// Decode one datagram, validating every field against the frozen
        /// schema. Reads strictly in wire order (little-endian), repacks the
        /// head quaternion from (w, x, y, z) into Unity's (x, y, z, w), and
        /// rejects trailing bytes.
        /// </summary>
        /// <param name="payload">Raw datagram bytes as sent by the Python streamer.</param>
        /// <param name="snapshot">The decoded snapshot when this returns true; null otherwise.</param>
        /// <returns>True if the payload is a conforming v1 SceneState; false for any violation (bad magic/version, truncation, oversized counts or labels, trailing bytes).</returns>
        public static bool TryDecode(byte[] payload, out SceneSnapshot snapshot)
        {
            snapshot = null;
            if (payload == null || payload.Length < HeaderSize)
            {
                return false;
            }

            using var stream = new MemoryStream(payload);
            using var reader = new BinaryReader(stream, Encoding.UTF8);

            var magic = reader.ReadBytes(4);
            for (int i = 0; i < 4; i++)
            {
                if (magic[i] != Magic[i])
                {
                    return false;
                }
            }

            byte version = reader.ReadByte();
            if (version != SchemaVersion)
            {
                return false;
            }

            byte flags = reader.ReadByte();
            uint sequence = reader.ReadUInt32();
            double timestamp = reader.ReadDouble();
            float hx = reader.ReadSingle();
            float hy = reader.ReadSingle();
            float hz = reader.ReadSingle();
            float qw = reader.ReadSingle();
            float qx = reader.ReadSingle();
            float qy = reader.ReadSingle();
            float qz = reader.ReadSingle();
            ushort count = reader.ReadUInt16();
            ushort beaconId = reader.ReadUInt16();

            if (count > MaxObjects)
            {
                return false;
            }

            var objects = new List<SceneObjectSnapshot>(count);
            for (int i = 0; i < count; i++)
            {
                if (stream.Length - stream.Position < 4)
                {
                    return false;
                }

                ushort objectId = reader.ReadUInt16();
                byte classId = reader.ReadByte();
                byte labelLen = reader.ReadByte();
                if (labelLen > MaxLabelBytes)
                {
                    return false;
                }
                if (stream.Length - stream.Position < labelLen + 24)
                {
                    return false;
                }

                string label = Encoding.UTF8.GetString(reader.ReadBytes(labelLen));
                float azimuth = reader.ReadSingle();
                float elevation = reader.ReadSingle();
                float distance = reader.ReadSingle();
                float wx = reader.ReadSingle();
                float wy = reader.ReadSingle();
                float wz = reader.ReadSingle();

                objects.Add(new SceneObjectSnapshot
                {
                    ObjectId = objectId,
                    ClassId = classId,
                    Label = label,
                    AzimuthDeg = azimuth,
                    ElevationDeg = elevation,
                    DistanceM = distance,
                    WorldPosition = new Vector3(wx, wy, wz),
                });
            }

            if (stream.Position != stream.Length)
            {
                return false;
            }

            snapshot = new SceneSnapshot
            {
                Sequence = sequence,
                Timestamp = timestamp,
                HeadValid = (flags & FlagHeadValid) != 0,
                HeadPosition = new Vector3(hx, hy, hz),
                HeadRotation = new Quaternion(qx, qy, qz, qw),
                BeaconActive = (flags & FlagBeacon) != 0,
                BeaconObjectId = beaconId,
                Objects = objects,
            };
            return true;
        }
    }
}
