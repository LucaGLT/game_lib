"""GmTrisBoardModule — the gmMap module adapted into a clickable Tris board.

The generic :class:`GmMapModule` renders a read-only spatial graph, which is
not suitable for *playing* Tic-Tac-Toe.  This module plays the same role
(visualising the ``gmMap`` board state) but exposes a clickable 3x3 grid so the
user can submit moves.  It is the interactive counterpart of ``GmMapModule``
inside the Tris hybrid GUI.

It consumes the engine's **native** Tris event contract directly (no adapter
translation) and emits ``gmTris.move`` / ``gmTris.new_game`` commands through
the injected :class:`EngineSender`.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget

from gmGui.modules.base_module import BaseModule

from widgets.board_widget import BoardWidget

# ── Engine event typeIds (native Tris contract) ───────────────────────────────
_EVT_MAP_SNAPSHOT = "gmMap.snapshot"
_EVT_CELL_CHANGED = "gmMap.cell_changed"
_EVT_SESSION_STARTED = "gmFlow.session.started"
_EVT_PHASE_CHANGED = "gmFlow.session.phase_changed"
_EVT_ACTOR_SNAPSHOT = "gmActor.snapshot"
_EVT_STATUS_ADDED = "gmActor.actor.status_added"
_EVT_GAME_WON = "gmRules.game_won"
_EVT_GAME_DRAW = "gmRules.game_draw"
_EVT_INVALID_MOVE = "gmTris.invalid_move"

# ── Command typeIds understood by the C++ CmdServer ───────────────────────────
_CMD_MOVE = "gmTris.move"

_STATUS_ACTIVE_TURN = "ACTIVE_TURN"


def _mark_of(actor_id: str) -> str:
    """Returns ``"X"`` or ``"O"`` from an actor id such as ``"Player_X"``."""
    return "X" if actor_id.endswith("X") else "O"


class GmTrisBoardModule(BaseModule):
    """Interactive 3x3 board module (the playable adaptation of GmMapModule)."""

    def __init__(self) -> None:
        super().__init__()
        self._board: BoardWidget | None = None
        self._status: QLabel | None = None
        self._active_mark: str | None = None
        self._game_over: bool = False

    # ── Identity / layout ─────────────────────────────────────────────────────

    @property
    def module_id(self) -> str:
        return "gm_tris_board"

    @property
    def title(self) -> str:
        return "Tabellone"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.LeftDockWidgetArea

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        container = QWidget()
        vbox = QVBoxLayout(container)

        self._status = QLabel("Premi «Nuova partita» per iniziare.")
        self._status.setObjectName("tris_board_status")
        self._status.setProperty("text_role", "subtitle")
        self._status.setAlignment(Qt.AlignmentFlag.AlignCenter)
        vbox.addWidget(self._status)

        self._board = BoardWidget(size=3)
        self._board.cell_clicked.connect(self._on_cell_clicked)
        vbox.addWidget(self._board, 1)

        return container

    # ── Engine bridge ─────────────────────────────────────────────────────────

    def subscribed_type_ids(self) -> list[str]:
        return [
            _EVT_MAP_SNAPSHOT,
            _EVT_CELL_CHANGED,
            _EVT_SESSION_STARTED,
            _EVT_PHASE_CHANGED,
            _EVT_ACTOR_SNAPSHOT,
            _EVT_STATUS_ADDED,
            _EVT_GAME_WON,
            _EVT_GAME_DRAW,
            _EVT_INVALID_MOVE,
        ]

    def on_envelope(self, msg: dict) -> None:
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {}) or {}

        if tid == _EVT_MAP_SNAPSHOT:
            self._on_map_snapshot(data)
        elif tid == _EVT_CELL_CHANGED:
            self._on_cell_changed(data)
        elif tid == _EVT_SESSION_STARTED:
            self._on_session_started(data)
        elif tid == _EVT_PHASE_CHANGED:
            self._on_phase_changed(data)
        elif tid == _EVT_ACTOR_SNAPSHOT:
            self._on_actor_snapshot(data)
        elif tid == _EVT_STATUS_ADDED:
            self._on_status_added(data)
        elif tid == _EVT_GAME_WON:
            self._on_game_won(data)
        elif tid == _EVT_GAME_DRAW:
            self._on_game_draw(data)
        elif tid == _EVT_INVALID_MOVE:
            self._on_invalid_move(data)

    # ── User actions ──────────────────────────────────────────────────────────

    def _on_cell_clicked(self, row: int, col: int) -> None:
        if self._game_over or self._active_mark is None:
            self._set_status("Nessuna partita in corso: premi «Nuova partita».")
            return
        self.send_command(_CMD_MOVE, {"player": self._active_mark, "row": row, "col": col})

    # ── Event handlers ────────────────────────────────────────────────────────

    def _on_map_snapshot(self, data: dict) -> None:
        if self._board is None:
            return
        self._board.reset()
        for cell in data.get("cells", []):
            self._board.set_cell(cell["row"], cell["col"], cell.get("mark", ""))

    def _on_cell_changed(self, data: dict) -> None:
        if self._board is None:
            return
        self._board.set_cell(
            int(data.get("row", 0)), int(data.get("col", 0)), str(data.get("mark", ""))
        )

    def _on_session_started(self, data: dict) -> None:
        self._game_over = False
        if self._board is not None:
            self._board.reset()
            self._board.set_enabled(True)
        self._set_status("Partita iniziata.")

    def _on_phase_changed(self, data: dict) -> None:
        if str(data.get("phase", "")) == "GAME_OVER":
            self._game_over = True
            if self._board is not None:
                self._board.set_enabled(False)

    def _on_actor_snapshot(self, data: dict) -> None:
        for actor in data.get("actors", []):
            if _STATUS_ACTIVE_TURN in actor.get("statuses", []):
                self._active_mark = _mark_of(str(actor.get("actor_id", "")))
                self._set_status(f"Turno del giocatore {self._active_mark}.")

    def _on_status_added(self, data: dict) -> None:
        if str(data.get("status", "")) == _STATUS_ACTIVE_TURN:
            self._active_mark = _mark_of(str(data.get("actor_id", "")))
            self._set_status(f"Turno del giocatore {self._active_mark}.")

    def _on_game_won(self, data: dict) -> None:
        mark: str = str(data.get("player", ""))
        line: str = str(data.get("line", ""))
        self._game_over = True
        if self._board is not None:
            self._board.highlight_line(line)
            self._board.set_enabled(False)
        self._set_status(f"Vince il giocatore {mark}!")

    def _on_game_draw(self, data: dict) -> None:
        self._game_over = True
        if self._board is not None:
            self._board.set_enabled(False)
        self._set_status("Partita pareggiata.")

    def _on_invalid_move(self, data: dict) -> None:
        reason: str = str(data.get("reason", "sconosciuto"))
        self._set_status(f"Mossa non valida: {reason}.")

    # ── Internal ──────────────────────────────────────────────────────────────

    def _set_status(self, text: str) -> None:
        if self._status is not None:
            self._status.setText(text)
