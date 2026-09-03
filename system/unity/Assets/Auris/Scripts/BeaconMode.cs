using UnityEngine;

namespace Auris
{
    /// <summary>
    /// Phase-2 interaction stub for beacon mode ("find the bottle"). For now
    /// a key press selects the query target; the actual pinging is driven
    /// upstream — Python sets the beacon id in the SceneState (FLAG_BEACON),
    /// and the audio side reacts via
    /// <see cref="SpatialAudioManager.TrySetBeaconClip"/>. Speech input
    /// replaces the keyboard in a later Phase-2 step.
    /// </summary>
    public sealed class BeaconMode : MonoBehaviour
    {
        /// <summary>Audio manager whose sources receive beacon clips; required.</summary>
        [SerializeField] private SpatialAudioManager audioManager;

        /// <summary>Keyboard key that issues the query (keyboard-first Phase 2).</summary>
        [SerializeField] private KeyCode queryKey = KeyCode.B;

        /// <summary>Object id used as the query target until speech/selection exists.</summary>
        [SerializeField] private ushort defaultTargetId;

        /// <summary>
        /// Object id currently queried, or null when no beacon is active.
        /// Consumers read this to highlight the target or arm the ping clip.
        /// </summary>
        public ushort? TargetId { get; private set; }

        /// <summary>Poll for the query key and arm the beacon target on press.</summary>
        private void Update()
        {
            if (Input.GetKeyDown(queryKey))
            {
                TargetId = defaultTargetId;
            }
        }
    }
}
