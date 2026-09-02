using System;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;

namespace Auris
{
    /// <summary>
    /// UDP listener that decodes SceneState v1 datagrams into scene snapshots.
    /// The Unity side of the frozen Python→Unity wire: a background thread
    /// receives datagrams, <see cref="SceneStateDecoder"/> validates them
    /// byte-for-byte, and conforming snapshots are queued for main-thread
    /// consumption by <see cref="SpatialAudioManager"/>. Malformed packets
    /// are counted, never thrown — a bad datagram is simply dropped.
    /// </summary>
    public sealed class SceneStateReceiver : MonoBehaviour
    {
        /// <summary>Local address to bind the UDP listener to (loopback in the Prototype baseline).</summary>
        [SerializeField] private string bindAddress = "127.0.0.1";

        /// <summary>UDP port; must match the Python streamer's destination (<c>auris stream --port</c>).</summary>
        [SerializeField] private int port = 5555;

        /// <summary>Thread-safe handoff queue: the listen thread enqueues, the main thread dequeues.</summary>
        private readonly ConcurrentQueue<SceneSnapshot> incoming = new ConcurrentQueue<SceneSnapshot>();

        /// <summary>Socket owned by this component; closed on disable.</summary>
        private UdpClient client;

        /// <summary>Background receive loop; runs for the component's enabled lifetime.</summary>
        private Thread listenThread;

        /// <summary>Set false on disable so the listen thread exits its loop.</summary>
        private volatile bool running;

        /// <summary>Count of datagrams lost to socket errors or failed decodes.</summary>
        private int rejectedPackets;

        /// <summary>
        /// Packets rejected since enable — socket errors or payloads that
        /// failed strict decoding. A non-zero steady value means the Python
        /// and Unity schema versions disagree.
        /// </summary>
        public int RejectedPackets => rejectedPackets;

        /// <summary>Snapshots currently waiting for main-thread consumption (a lag gauge).</summary>
        public int QueuedSnapshots => incoming.Count;

        /// <summary>
        /// Try to take the next decoded snapshot off the handoff queue.
        /// Called from the main thread (e.g. <c>Update</c> of a consumer).
        /// </summary>
        /// <param name="snapshot">The dequeued snapshot when this returns true.</param>
        /// <returns>True if a snapshot was available; false if the queue was empty.</returns>
        public bool TryDequeue(out SceneSnapshot snapshot)
        {
            return incoming.TryDequeue(out snapshot);
        }

        /// <summary>Bind the socket and start the background receive thread.</summary>
        private void OnEnable()
        {
            client = new UdpClient(new IPEndPoint(IPAddress.Parse(bindAddress), port));
            running = true;
            listenThread = new Thread(ListenLoop)
            {
                IsBackground = true,
                Name = "auris-scenestate-udp",
            };
            listenThread.Start();
        }

        /// <summary>Stop the receive thread and release the socket; safe across repeated disables.</summary>
        private void OnDisable()
        {
            running = false;
            if (client != null)
            {
                client.Close();
                client = null;
            }
            if (listenThread != null && listenThread.IsAlive)
            {
                listenThread.Join(500);
                listenThread = null;
            }
        }

        /// <summary>
        /// Background loop: block on receive, strictly decode each datagram,
        /// enqueue conforming snapshots, and count everything else. Exits
        /// when <c>running</c> goes false or the socket is closed beneath it.
        /// </summary>
        private void ListenLoop()
        {
            while (running)
            {
                IPEndPoint remote = null;
                byte[] payload;
                try
                {
                    payload = client.Receive(ref remote);
                }
                catch (SocketException) when (!running)
                {
                    break;
                }
                catch (ObjectDisposedException)
                {
                    break;
                }
                catch (Exception)
                {
                    if (running)
                    {
                        Interlocked.Increment(ref rejectedPackets);
                    }
                    continue;
                }

                if (SceneStateDecoder.TryDecode(payload, out SceneSnapshot snapshot))
                {
                    incoming.Enqueue(snapshot);
                }
                else
                {
                    Interlocked.Increment(ref rejectedPackets);
                }
            }
        }
    }
}
