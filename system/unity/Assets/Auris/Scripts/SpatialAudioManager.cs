using System.Collections.Generic;
using UnityEngine;

namespace Auris
{
    /// <summary>
    /// The "dumb" audio renderer: places one looping spatialized AudioSource
    /// per tracked object at its head-relative position. All spatial math
    /// happened upstream in Python (conventions.md) — this component never
    /// computes poses; it drains <see cref="SceneStateReceiver"/>'s queue,
    /// creates/destroys sources as objects appear and vanish, and repositions
    /// each source around the head anchor every snapshot.
    /// </summary>
    public sealed class SpatialAudioManager : MonoBehaviour
    {
        /// <summary>Receiver component to drain snapshots from; required.</summary>
        [SerializeField] private SceneStateReceiver receiver;

        /// <summary>Transform the listener follows (the user's head); sources are placed relative to it.</summary>
        [SerializeField] private Transform headAnchor;

        /// <summary>Default earcon looped by every new source until overridden per object.</summary>
        [SerializeField] private AudioClip earcon;

        /// <summary>Distance clamp, meters: closer sources snap here (avoids deafening proximity).</summary>
        [SerializeField] private float minDistanceMeters = 0.3f;

        /// <summary>Distance clamp, meters: farther sources snap here (keeps faint earcons audible).</summary>
        [SerializeField] private float maxDistanceMeters = 20.0f;

        /// <summary>Live sources keyed by object id; one GameObject + AudioSource per tracked object.</summary>
        private readonly Dictionary<ushort, AudioSource> sources = new Dictionary<ushort, AudioSource>();

        /// <summary>
        /// Replace the looping clip of an existing source — the hook beacon
        /// mode uses to make the target object emit a distinct ping.
        /// </summary>
        /// <param name="objectId">Object whose source to re-clip.</param>
        /// <param name="clip">New clip for that source.</param>
        /// <returns>True if the object currently has a live source; false if the id is unknown.</returns>
        public bool TrySetBeaconClip(ushort objectId, AudioClip clip)
        {
            if (sources.TryGetValue(objectId, out AudioSource source) && source != null)
            {
                source.clip = clip;
                return true;
            }
            return false;
        }

        /// <summary>
        /// Per-frame pump: drain every queued snapshot (latest wins by
        /// exhaustion) and, for head-valid snapshots only, reposition each
        /// object's source and prune sources whose objects disappeared.
        /// Head-invalid snapshots are skipped entirely — hold last-good.
        /// </summary>
        private void Update()
        {
            if (receiver == null || headAnchor == null)
            {
                return;
            }

            while (receiver.TryDequeue(out SceneSnapshot snapshot))
            {
                if (snapshot.HeadValid)
                {
                    foreach (SceneObjectSnapshot obj in snapshot.Objects)
                    {
                        UpdateSource(obj, snapshot);
                    }
                    PruneMissing(snapshot);
                }
            }
        }

        /// <summary>
        /// Create or reposition one object's source: convert the snapshot's
        /// azimuth/elevation/distance into a direction, clamp the distance,
        /// and place the source at head + rotated direction × distance.
        /// </summary>
        /// <param name="obj">The object snapshot to render.</param>
        /// <param name="snapshot">Owning snapshot (reserved for future per-snapshot state).</param>
        private void UpdateSource(SceneObjectSnapshot obj, SceneSnapshot snapshot)
        {
            if (!sources.TryGetValue(obj.ObjectId, out AudioSource source) || source == null)
            {
                source = CreateSource(obj.ObjectId, obj.Label);
                sources[obj.ObjectId] = source;
            }

            Vector3 direction = AzimuthElevationToDirection(obj.AzimuthDeg, obj.ElevationDeg);
            float distance = Mathf.Clamp(obj.DistanceM, minDistanceMeters, maxDistanceMeters);
            source.transform.position = headAnchor.position + headAnchor.rotation * direction * distance;
        }

        /// <summary>
        /// Spawn the GameObject + AudioSource for one object: looping,
        /// fully spatialized (spatialBlend 1), linear rolloff between the
        /// configured distance clamps, autoplaying when a clip is assigned.
        /// </summary>
        /// <param name="objectId">Object id, used for a stable GameObject name.</param>
        /// <param name="label">Object label, used for a readable GameObject name.</param>
        /// <returns>The new component, parented under this manager.</returns>
        private AudioSource CreateSource(ushort objectId, string label)
        {
            var host = new GameObject($"earcon-{objectId:D3}-{label}");
            host.transform.SetParent(transform, worldPositionStays: false);
            AudioSource source = host.AddComponent<AudioSource>();
            source.clip = earcon;
            source.loop = true;
            source.playOnAwake = false;
            source.spatialBlend = 1.0f;
            source.spatialize = true;
            source.rolloffMode = AudioRolloffMode.Linear;
            source.minDistance = minDistanceMeters;
            source.maxDistance = maxDistanceMeters;
            if (source.clip != null)
            {
                source.Play();
            }
            return source;
        }

        /// <summary>
        /// Destroy sources whose object ids are absent from the snapshot, so
        /// vanished objects stop sounding instead of freezing at a stale
        /// position.
        /// </summary>
        /// <param name="snapshot">The newest head-valid snapshot defining the live object set.</param>
        private void PruneMissing(SceneSnapshot snapshot)
        {
            var present = new HashSet<ushort>();
            foreach (SceneObjectSnapshot obj in snapshot.Objects)
            {
                present.Add(obj.ObjectId);
            }

            List<ushort> stale = null;
            foreach (KeyValuePair<ushort, AudioSource> pair in sources)
            {
                if (!present.Contains(pair.Key))
                {
                    (stale ??= new List<ushort>()).Add(pair.Key);
                }
            }

            if (stale == null)
            {
                return;
            }
            foreach (ushort objectId in stale)
            {
                Destroy(sources[objectId].gameObject);
                sources.Remove(objectId);
            }
        }

        /// <summary>
        /// Convert head-relative spherical coordinates into a unit direction
        /// vector in head-anchor local space — the Unity-side inverse of the
        /// Python transform. The convention matches conventions.md rotated
        /// into Unity axes: azimuth 0 looks down local +z, positive azimuth
        /// swings toward local +x, positive elevation toward local +y.
        /// </summary>
        /// <param name="azimuthDeg">Head-relative azimuth in degrees (positive to the user's right).</param>
        /// <param name="elevationDeg">Head-relative elevation in degrees (positive above the horizon).</param>
        /// <returns>Unit direction vector in head-anchor local space.</returns>
        internal static Vector3 AzimuthElevationToDirection(float azimuthDeg, float elevationDeg)
        {
            float azimuth = azimuthDeg * Mathf.Deg2Rad;
            float elevation = elevationDeg * Mathf.Deg2Rad;
            float cosElevation = Mathf.Cos(elevation);
            return new Vector3(
                Mathf.Sin(azimuth) * cosElevation,
                Mathf.Sin(elevation),
                Mathf.Cos(azimuth) * cosElevation
            );
        }
    }
}
