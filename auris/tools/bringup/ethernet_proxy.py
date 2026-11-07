"""Serve the Pi dashboard locally while binding requests to the Ethernet link.

Run this on the Windows PC when the Pi is reachable through a specific local
Ethernet address but the browser chooses a different network interface. The
proxy listens on loopback only and forwards the dashboard, JSON snapshots, and
annotated MJPEG stream to one configured Pi address.
"""

from __future__ import annotations

import argparse
import ipaddress
from http.client import HTTPConnection, HTTPException
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit


_HOP_BY_HOP_HEADERS = {
    "connection",
    "keep-alive",
    "proxy-authenticate",
    "proxy-authorization",
    "te",
    "trailer",
    "transfer-encoding",
    "upgrade",
}


class AurisEthernetProxyServer(ThreadingHTTPServer):
    """Forward loopback dashboard requests over a selected Pi Ethernet path.

    Own the local HTTP listener and the fixed upstream address configuration.
    The listener is intentionally pinned to 127.0.0.1, while every upstream
    socket binds to the configured Windows Ethernet IPv4 address.
    """

    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, port: int, pi_address: str, pi_port: int,
                 source_ip: str) -> None:
        """Bind the loopback listener and store the Pi route details.

        Args:
            port: Local TCP port for the browser, from 1 through 65535.
            pi_address: Pi IPv4 address reachable over the direct Ethernet link.
            pi_port: Pi dashboard TCP port, usually 8081.
            source_ip: Windows Ethernet IPv4 address used for upstream sockets.

        Raises:
            OSError: If the loopback port is unavailable.
            ValueError: If any configured TCP port is outside its valid range.
        """
        if not 1 <= port <= 65535 or not 1 <= pi_port <= 65535:
            raise ValueError("TCP ports must be between 1 and 65535")
        self.pi_address = pi_address
        self.pi_port = pi_port
        self.source_ip = source_ip
        super().__init__(("127.0.0.1", port), AurisEthernetProxyHandler)


class AurisEthernetProxyHandler(BaseHTTPRequestHandler):
    """Forward read-only dashboard requests and stream camera frames promptly.

    Use the fixed upstream stored by :class:`AurisEthernetProxyServer`; never
    accept an upstream hostname or port from an HTTP request.
    """

    protocol_version = "HTTP/1.0"
    server_version = "AurisEthernetProxy/1.0"
    sys_version = ""

    def do_GET(self) -> None:
        """Forward one dashboard GET request and stream MJPEG frames promptly.

        Returns:
            None. Write the upstream status, headers and body to the browser.

        Raises:
            None. Upstream failures become HTTP 502 responses when possible.
        """
        self._forward("GET", include_body=True)

    def do_HEAD(self) -> None:
        """Forward one dashboard HEAD request without copying a response body.

        Returns:
            None. Write the upstream status and headers to the browser.

        Raises:
            None. Upstream failures become HTTP 502 responses when possible.
        """
        self._forward("HEAD", include_body=False)

    def _forward(self, method: str, include_body: bool) -> None:
        """Bind one upstream request to Ethernet and relay its response.

        Args:
            method: The allow-listed HTTP method, ``GET`` or ``HEAD``.
            include_body: Whether to copy the upstream response body.

        Returns:
            None. MJPEG responses remain open until the browser disconnects.

        Raises:
            No upstream ``OSError`` or ``HTTPException`` escapes; failures
            become HTTP 502 responses unless headers have started streaming.
        """
        target = urlsplit(self.path)
        if target.scheme or target.netloc or not target.path.startswith("/"):
            self.send_error(400, "Expected a dashboard path")
            return

        request_path = target.path
        if target.query:
            request_path += f"?{target.query}"

        upstream = HTTPConnection(
            self.server.pi_address,
            self.server.pi_port,
            timeout=5.0,
            source_address=(self.server.source_ip, 0),
        )
        headers = {
            name: value
            for name, value in self.headers.items()
            if name.lower() not in _HOP_BY_HOP_HEADERS
            and name.lower() not in {"host", "accept-encoding", "content-length"}
        }
        headers["Accept-Encoding"] = "identity"
        headers["Connection"] = "close"
        headers["Host"] = f"{self.server.pi_address}:{self.server.pi_port}"
        headers_sent = False

        try:
            upstream.request(method, request_path, headers=headers)
            response = upstream.getresponse()
            self.send_response(response.status, response.reason)
            for name, value in response.getheaders():
                if name.lower() not in _HOP_BY_HOP_HEADERS | {"date", "server"}:
                    self.send_header(name, value)
            self.end_headers()
            headers_sent = True

            if not include_body:
                return
            content_type = response.getheader("Content-Type", "").lower()
            if content_type.startswith("multipart/x-mixed-replace"):
                while chunk := response.read1(16_384):
                    self.wfile.write(chunk)
                    self.wfile.flush()
            else:
                self.wfile.write(response.read())
        except (OSError, HTTPException) as exc:
            if not headers_sent:
                self.send_error(502, f"Pi dashboard unavailable: {exc}")
        finally:
            upstream.close()

    def log_message(self, format_string: str, *args: object) -> None:
        """Suppress per-poll access logs to keep the console readable.

        Args:
            format_string: Base handler's format string for one access record.
            args: Values that would be interpolated into that record.

        Returns:
            None. Request failures remain visible through HTTP error responses.

        Raises:
            None.
        """
        return


def parse_ipv4(value: str) -> str:
    """Validate and normalize one IPv4 address supplied on the command line.

    Args:
        value: Dotted-decimal address for the Pi or Windows Ethernet interface.

    Returns:
        The normalized dotted-decimal IPv4 address.

    Raises:
        argparse.ArgumentTypeError: If the value is not an IPv4 address.
    """
    try:
        address = ipaddress.ip_address(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("expected an IPv4 address") from exc
    if address.version != 4:
        raise argparse.ArgumentTypeError("expected an IPv4 address")
    return str(address)


def build_parser() -> argparse.ArgumentParser:
    """Define the loopback proxy and its fixed Pi Ethernet destination.

    Returns:
        An argument parser for the Pi address, source interface and local port.
    """
    parser = argparse.ArgumentParser(
        description="Proxy the Auris Pi dashboard over a selected Ethernet interface.",
    )
    parser.add_argument("--pi-address", type=parse_ipv4, default="192.168.77.2",
                        help="Pi Ethernet IPv4 address (default: 192.168.77.2)")
    parser.add_argument("--pi-port", type=int, default=8081,
                        help="Pi dashboard port (default: 8081)")
    parser.add_argument("--source-ip", type=parse_ipv4, required=True,
                        help="Windows Ethernet IPv4 address, for example 192.168.77.1")
    parser.add_argument("--local-port", type=int, default=8781,
                        help="Loopback browser port (default: 8781)")
    return parser


def main() -> int:
    """Run the local-only proxy until the user stops it with Ctrl+C.

    Returns:
        Zero after a normal keyboard interruption.

    Raises:
        OSError: If the Pi route or loopback listening port cannot be opened.
        ValueError: If a configured TCP port is outside its valid range.
    """
    args = build_parser().parse_args()
    server = AurisEthernetProxyServer(
        port=args.local_port,
        pi_address=args.pi_address,
        pi_port=args.pi_port,
        source_ip=args.source_ip,
    )
    print(
        f"Auris Ethernet view: http://127.0.0.1:{args.local_port}/ "
        f"-> http://{args.pi_address}:{args.pi_port}/ via {args.source_ip}; "
        "press Ctrl+C to stop",
        flush=True,
    )
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
