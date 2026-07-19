"""fake_engine — minimal stand-in "engine" process for gmWebServe's own tests.

Speaks the exact same wire format every real gmXxx engine uses (4-byte
big-endian length prefix + UTF-8 JSON, ``headers["data"]`` JSON-string
payload — see ``pyLib/gmGui/engine_bridge/framing.py``), so
:class:`gmWebServe.session_registry.SessionRegistry` can be exercised
end-to-end without needing any real C++ engine executable built first.

CLI contract mirrors the convention added to the real engines (Tris/Eldhom):
``--events-port <port> --commands-port <port>``. Like every real engine, it
connects OUT to the events port (client role, matching ``GuiBridge``) and
listens on the commands port (server role, matching ``CmdServer``).
"""
from __future__ import annotations

import argparse
import json
import socket
import sys
import threading
from pathlib import Path

_GMGUI_DIR = Path(__file__).resolve().parents[2] / "gmGui"
if str(_GMGUI_DIR) not in sys.path:
    sys.path.append(str(_GMGUI_DIR))

from engine_bridge.framing import recv_frame, send_frame  # noqa: E402


def _send_event(events_port: int, type_id: str, data: dict) -> None:
    """Connects to the events listener and sends one envelope, then disconnects."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5.0)
    sock.connect(("127.0.0.1", events_port))
    try:
        envelope = {"typeId": type_id, "source": "FakeEngine", "headers": {"data": json.dumps(data)}}
        send_frame(sock, json.dumps(envelope))
    finally:
        sock.close()


def _handle_client(client: socket.socket, events_port: int) -> None:
    """Reads command frames from *client* and echoes each one back as an event."""
    client.settimeout(0.5)
    while True:
        try:
            raw = recv_frame(client)
        except socket.timeout:
            continue
        except (ConnectionError, OSError):
            return
        message = json.loads(raw)
        _send_event(
            events_port,
            "fake.echo",
            {"received_type_id": message.get("typeId"), "received_data": message.get("data", {})},
        )


def main() -> None:
    """Parses argv, announces startup, then serves commands until killed."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--events-port", type=int, required=True)
    parser.add_argument("--commands-port", type=int, required=True)
    args, _unknown = parser.parse_known_args()

    _send_event(args.events_port, "fake.started", {})

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", args.commands_port))
    server.listen(5)
    while True:
        client, _addr = server.accept()
        threading.Thread(target=_handle_client, args=(client, args.events_port), daemon=True).start()


if __name__ == "__main__":
    main()
