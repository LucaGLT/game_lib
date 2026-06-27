"""Le Pergamene di Eldhom — engine bridge adapter.

EldhomBridge wraps the shared ``gmGui.engine_bridge`` framing layer:

- ``receiver``: event server on port 9210 (GUI is TCP server).
- ``sender``:   command client on port 9211 (C++ engine is server).
- ``send_command(type_id, data)``: helper to send one command.

Wire format: 4-byte big-endian length prefix + UTF-8 JSON.
"""
from __future__ import annotations

import sys
from pathlib import Path
from typing import Callable

_PYLIB_DIR = Path(__file__).resolve().parents[3] / "pyLib"
if str(_PYLIB_DIR) not in sys.path:
    sys.path.append(str(_PYLIB_DIR))

from gmGui.engine_bridge.receiver import EngineReceiver  # noqa: E402
from gmGui.engine_bridge.sender import EngineSender  # noqa: E402

_EVENTS_PORT   = 9210   # GUI is TCP server; engine connects as client
_COMMANDS_PORT = 9211   # Engine is TCP server; GUI connects as client


class EldhomBridge:
    """Thin wrapper around shared engine_bridge infrastructure for Eldhom."""

    def __init__(self) -> None:
        """Creates receiver and sender but does not yet start them."""
        self._receiver = EngineReceiver(port=_EVENTS_PORT)
        self._sender   = EngineSender(host="127.0.0.1", port=_COMMANDS_PORT)

    @property
    def receiver(self) -> EngineReceiver:
        """The event receiver (listens on port 9210)."""
        return self._receiver

    @property
    def sender(self) -> EngineSender:
        """The command sender (connects to port 9211)."""
        return self._sender

    def send_command(self, type_id: str, data: dict) -> None:
        """Sends a single command to the CoreEngine.

        Args:
            type_id: Command typeId string (e.g. ``"eldhom.play_card"``).
            data:    Command payload dict.
        """
        self._sender.send_command(type_id, data)

    def set_on_event(self, handler: Callable[[dict], None]) -> None:
        """Registers the callback invoked for every incoming engine event.

        Args:
            handler: Callable receiving the decoded event dict.
        """
        self._receiver.on_message = handler
