"""TrisBridge — high-level transport adapter for the Tic-Tac-Toe GUI.

Reuses the existing ``gmGui.engine_bridge`` transport (framing / receiver /
sender) without pulling in the rest of the gmGui application. The folder
``pyLib/gmGui`` is added to ``sys.path`` so that ``engine_bridge`` can be
imported as a top-level package (its modules use relative imports internally).

Responsibilities:
    - Run an :class:`EngineReceiver` server on port 9000 (engine → GUI events).
    - Own an :class:`EngineSender` client to port 9001 (GUI → engine commands).
    - Re-emit every received envelope as :attr:`envelope_received`.
    - Offer typed helpers :meth:`send_move` and :meth:`send_new_game`.
"""
from __future__ import annotations

import sys
from pathlib import Path

from PySide6.QtCore import QObject, Signal

# ── Make the shared engine_bridge package importable ──────────────────────────
# Appended (not prepended) so the GUI's own ``app`` / ``widgets`` packages keep
# priority over the similarly named packages inside ``pyLib/gmGui``.
_GMGUI_DIR = Path(__file__).resolve().parents[4] / "pyLib" / "gmGui"
if _GMGUI_DIR.is_dir() and str(_GMGUI_DIR) not in sys.path:
    sys.path.append(str(_GMGUI_DIR))

from engine_bridge.receiver import EngineReceiver  # noqa: E402
from engine_bridge.sender import EngineSender  # noqa: E402

# Command typeIds understood by the C++ CmdServer.
_CMD_MOVE = "gmTris.move"
_CMD_NEW_GAME = "gmTris.new_game"


class TrisBridge(QObject):
    """Couples the inbound event receiver and the outbound command sender."""

    envelope_received: Signal = Signal(dict)
    connection_lost: Signal = Signal()

    def __init__(self, event_port: int = 9100, command_port: int = 9001) -> None:
        super().__init__()
        self._receiver: EngineReceiver = EngineReceiver(port=event_port)
        self._sender: EngineSender = EngineSender(port=command_port)
        self._receiver.envelope_received.connect(self.envelope_received)
        self._receiver.connection_lost.connect(self.connection_lost)

    def start(self) -> None:
        """Starts the receiver server thread (waits for the engine to connect)."""
        self._receiver.start()

    def stop(self) -> None:
        """Stops the receiver thread and closes the sender connection."""
        self._receiver.stop()
        self._sender.close()

    def send_move(self, player: str, row: int, col: int) -> None:
        """Sends a ``gmTris.move`` command for *player* at (*row*, *col*)."""
        self._sender.send_command(
            _CMD_MOVE, {"player": player, "row": row, "col": col}
        )

    def send_new_game(self, starter_mode: str = "fixed_x") -> None:
        """Sends a ``gmTris.new_game`` command with the chosen starter mode."""
        self._sender.send_command(_CMD_NEW_GAME, {"starter_mode": starter_mode})
