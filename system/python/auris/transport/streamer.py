"""SceneState UDP streamer — the Python side of the `AudioSink` contract.

Sends one datagram per update cycle to Unity over UDP (localhost in the
Prototype baseline; LAN under the two-box escalation rung — contract
unchanged, README §7.2).
"""

from __future__ import annotations

import socket

from auris.contracts.scene import SceneState

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 5555


class SceneStateStreamer:
    """Encodes SceneStates and fires them at a UDP endpoint.

    Purpose: let the same pipeline feed Unity in production or a loopback
    socket in tests — the `AudioSink` seam where Phase-3 latency
    instrumentation attaches. One datagram per update cycle; UDP's fire-and-
    forget semantics match the latest-wins snapshot model (a lost packet is
    simply the previous scene, superseded 30–60 times per second anyway).

    Attributes:
        packets_sent: Total datagrams handed to the socket since construction.
    """

    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT) -> None:
        """Open the UDP socket (unconnected; destinations are per-send).

        Args:
            host: Destination host — `127.0.0.1` for the localhost baseline,
                the LAN address of Desktop PC #2 under the two-box rung.
            port: Destination UDP port (Unity's `SceneStateReceiver` binds
                the same port).
        """
        self._host = host
        self._port = port
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.packets_sent = 0

    def update_scene(self, state: SceneState) -> None:
        """Encode and send one SceneState as a single UDP datagram.

        No throttling or batching happens here — the pipeline runner paces
        the 30–60 Hz rate.

        Args:
            state: The pipeline's newest scene snapshot, encoded per the
                frozen v1 schema.
        """
        payload = state.encode()
        self._socket.sendto(payload, (self._host, self._port))
        self.packets_sent += 1

    def close(self) -> None:
        """Release the socket; further `update_scene` calls would fail."""
        self._socket.close()

    def __enter__(self) -> SceneStateStreamer:
        """Enter the context; returns the streamer itself."""
        return self

    def __exit__(self, *exc_info: object) -> None:
        """Close the socket regardless of exception state."""
        self.close()
