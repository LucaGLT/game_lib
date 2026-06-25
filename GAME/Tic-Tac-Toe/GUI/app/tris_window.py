"""TrisWindow — main window for the Tic-Tac-Toe GUI.

Layout (matching the reference figure regions)::

    ┌─────────────────────────────────────────────────────────┐
    │ Header:  [Salvataggio]  [Reload]   Inizio: (X | Dado)    │
    ├─────────────────────────────────────────────────────────┤
    │ Stato del Giocatore di Turno                             │  (Body_Header)
    ├──────────────────────────────────┬──────────────────────┤
    │ Tabellone (3x3 cliccabile)       │ Log della partita    │  (Body | R_Panel)
    ├──────────────────────────────────┴──────────────────────┤
    │ Stato dei Turni                                         │  (Body_Footer)
    ├─────────────────────────────────────────────────────────┤
    │ Messaggi di errore                                      │  (Footer)
    └─────────────────────────────────────────────────────────┘

The left side panel (L_Panel) is intentionally unused, as in the figure.

All game logic lives in the C++ CoreEngine. This window only renders engine
events and turns user clicks into ``gmTris.move`` / ``gmTris.new_game`` commands.
"""
from __future__ import annotations

from PySide6.QtCore import Slot
from PySide6.QtWidgets import (
    QComboBox,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from app.tris_bridge import TrisBridge
from widgets.board_widget import BoardWidget
from widgets.error_bar_widget import ErrorBarWidget
from widgets.log_widget import LogWidget
from widgets.turn_state_widget import TurnFooterWidget, TurnHeaderWidget

# ── Engine event typeIds ──────────────────────────────────────────────────────
_EVT_ACTOR_SNAPSHOT = "gmActor.snapshot"
_EVT_MAP_SNAPSHOT = "gmMap.snapshot"
_EVT_SESSION_STARTED = "gmFlow.session.started"
_EVT_PHASE_CHANGED = "gmFlow.session.phase_changed"
_EVT_STATUS_ADDED = "gmActor.actor.status_added"
_EVT_STATUS_REMOVED = "gmActor.actor.status_removed"
_EVT_CELL_CHANGED = "gmMap.cell_changed"
_EVT_GAME_WON = "gmRules.game_won"
_EVT_GAME_DRAW = "gmRules.game_draw"
_EVT_INVALID_MOVE = "gmTris.invalid_move"
_EVT_DICE_ROLLED = "gmAlea.dice_rolled"

_STATUS_CONNECTED = "Engine: Connesso"
_STATUS_DISCONNECTED = "Engine: Disconnesso"


def _mark_of(actor_id: str) -> str:
    """Returns ``"X"`` or ``"O"`` from an actor id such as ``"Player_X"``."""
    return "X" if actor_id.endswith("X") else "O"


class TrisWindow(QMainWindow):
    """Application shell that wires the bridge to the Tris widgets."""

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Tic-Tac-Toe — GameLib")
        self.resize(900, 640)

        self._bridge: TrisBridge = TrisBridge()
        self._active_mark: str | None = None
        self._game_over: bool = False

        # Maps each engine typeId to its handler.
        self._handlers = {
            _EVT_ACTOR_SNAPSHOT: self._on_actor_snapshot,
            _EVT_MAP_SNAPSHOT: self._on_map_snapshot,
            _EVT_SESSION_STARTED: self._on_session_started,
            _EVT_PHASE_CHANGED: self._on_phase_changed,
            _EVT_STATUS_ADDED: self._on_status_added,
            _EVT_STATUS_REMOVED: self._on_status_removed,
            _EVT_CELL_CHANGED: self._on_cell_changed,
            _EVT_GAME_WON: self._on_game_won,
            _EVT_GAME_DRAW: self._on_game_draw,
            _EVT_INVALID_MOVE: self._on_invalid_move,
            _EVT_DICE_ROLLED: self._on_dice_rolled,
        }

        self._build_ui()
        self._wire_signals()

        self.statusBar().showMessage(_STATUS_DISCONNECTED)
        self._bridge.start()

    # ── UI construction ────────────────────────────────────────────────────────

    def _build_ui(self) -> None:
        central = QWidget()
        root = QVBoxLayout(central)

        # ── Header: Salvataggio / Reload / starter mode ───────────────────────
        header = QHBoxLayout()
        self._save_button = QPushButton("Salvataggio")
        self._reload_button = QPushButton("Reload")
        self._starter_combo = QComboBox()
        self._starter_combo.addItem("Inizio: X (fisso)", "fixed_x")
        self._starter_combo.addItem("Inizio: Dado 1d2", "dice_1d2")
        header.addWidget(self._save_button)
        header.addWidget(self._reload_button)
        header.addStretch(1)
        header.addWidget(QLabel("Modalità:"))
        header.addWidget(self._starter_combo)
        root.addLayout(header)

        # ── Body_Header: current player ───────────────────────────────────────
        self._turn_header = TurnHeaderWidget()
        header_box = QGroupBox("Stato del Giocatore di Turno")
        header_box_layout = QVBoxLayout(header_box)
        header_box_layout.addWidget(self._turn_header)
        root.addWidget(header_box)

        # ── Body + R_Panel: board and log ─────────────────────────────────────
        middle = QHBoxLayout()

        self._board = BoardWidget(size=3)
        board_box = QGroupBox("Tabellone")
        board_box_layout = QVBoxLayout(board_box)
        board_box_layout.addWidget(self._board)
        middle.addWidget(board_box, 2)

        self._log = LogWidget()
        log_box = QGroupBox("Log della partita")
        log_box_layout = QVBoxLayout(log_box)
        log_box_layout.addWidget(self._log)
        middle.addWidget(log_box, 1)

        root.addLayout(middle, 1)

        # ── Body_Footer: per-player status ────────────────────────────────────
        self._turn_footer = TurnFooterWidget()
        footer_box = QGroupBox("Stato dei Turni")
        footer_box_layout = QVBoxLayout(footer_box)
        footer_box_layout.addWidget(self._turn_footer)
        root.addWidget(footer_box)

        # ── Footer: error messages ────────────────────────────────────────────
        self._error_bar = ErrorBarWidget()
        error_box = QGroupBox("Messaggi di errore")
        error_box_layout = QVBoxLayout(error_box)
        error_box_layout.addWidget(self._error_bar)
        root.addWidget(error_box)

        self.setCentralWidget(central)

    def _wire_signals(self) -> None:
        self._bridge.envelope_received.connect(self._on_envelope)
        self._bridge.connection_lost.connect(self._on_connection_lost)
        self._board.cell_clicked.connect(self._on_cell_clicked)
        self._reload_button.clicked.connect(self._on_reload)
        self._save_button.clicked.connect(self._on_save)

    # ── User actions ───────────────────────────────────────────────────────────

    @Slot(int, int)
    def _on_cell_clicked(self, row: int, col: int) -> None:
        """Sends a move for the active player, if the game is running."""
        if self._game_over or self._active_mark is None:
            self._error_bar.show_error("Nessuna partita in corso: premi Reload.")
            return
        self._bridge.send_move(self._active_mark, row, col)

    @Slot()
    def _on_reload(self) -> None:
        """Requests a new match with the selected starter mode."""
        mode = self._starter_combo.currentData()
        self._game_over = False
        self._error_bar.clear_error()
        self._bridge.send_new_game(mode)
        self._log.append_line(f"Richiesta nuova partita (modalità: {mode}).")

    @Slot()
    def _on_save(self) -> None:
        """Placeholder for the save feature (planned for a later phase)."""
        self._error_bar.show_error("Salvataggio non ancora disponibile (Fase 4).")

    # ── Envelope routing ───────────────────────────────────────────────────────

    @Slot(dict)
    def _on_envelope(self, msg: dict) -> None:
        """Routes an engine envelope to the matching handler."""
        if self.statusBar().currentMessage() != _STATUS_CONNECTED:
            self.statusBar().showMessage(_STATUS_CONNECTED)
        handler = self._handlers.get(msg.get("typeId", ""))
        if handler is not None:
            handler(msg.get("data", {}))

    @Slot()
    def _on_connection_lost(self) -> None:
        self.statusBar().showMessage(_STATUS_DISCONNECTED)

    # ── Event handlers ─────────────────────────────────────────────────────────

    def _on_actor_snapshot(self, data: dict) -> None:
        actors = data.get("actors", [])
        self._turn_footer.set_players(actors)
        for actor in actors:
            if "ACTIVE_TURN" in actor.get("statuses", []):
                self._active_mark = _mark_of(actor.get("actor_id", ""))
                self._turn_header.set_turn(self._active_mark)

    def _on_map_snapshot(self, data: dict) -> None:
        self._board.reset()
        for cell in data.get("cells", []):
            self._board.set_cell(cell["row"], cell["col"], cell.get("mark", ""))

    def _on_session_started(self, data: dict) -> None:
        self._game_over = False
        self._error_bar.clear_error()
        self._board.set_enabled(True)
        self._log.append_line("Partita iniziata.")

    def _on_phase_changed(self, data: dict) -> None:
        phase = data.get("phase", "")
        if phase == "GAME_OVER":
            self._game_over = True
            self._board.set_enabled(False)

    def _on_status_added(self, data: dict) -> None:
        actor_id = data.get("actor_id", "")
        status = data.get("status", "")
        self._turn_footer.add_status(actor_id, status)
        if status == "ACTIVE_TURN":
            self._active_mark = _mark_of(actor_id)
            self._turn_header.set_turn(self._active_mark)

    def _on_status_removed(self, data: dict) -> None:
        self._turn_footer.remove_status(
            data.get("actor_id", ""), data.get("status", "")
        )

    def _on_cell_changed(self, data: dict) -> None:
        mark = data.get("mark", "")
        row = data.get("row", 0)
        col = data.get("col", 0)
        self._board.set_cell(row, col, mark)
        self._error_bar.clear_error()
        self._log.append_line(f"Player {mark} gioca in ({row}, {col}).")

    def _on_game_won(self, data: dict) -> None:
        mark = data.get("player", "")
        line = data.get("line", "")
        self._game_over = True
        self._board.highlight_line(line)
        self._board.set_enabled(False)
        self._turn_header.set_winner(mark)
        self._log.append_line(f"Player {mark} vince ({line}).")

    def _on_game_draw(self, data: dict) -> None:
        self._game_over = True
        self._board.set_enabled(False)
        self._turn_header.set_draw()
        self._log.append_line("Partita pareggiata.")

    def _on_invalid_move(self, data: dict) -> None:
        reason = data.get("reason", "sconosciuto")
        self._error_bar.show_error(f"Mossa non valida: {reason}.")
        self._log.append_line(f"Mossa rifiutata ({reason}).")

    def _on_dice_rolled(self, data: dict) -> None:
        value = data.get("value", "?")
        first = data.get("first", "?")
        self._log.append_line(f"Dado 1d2 = {value} → inizia Player {first}.")

    # ── Lifecycle ──────────────────────────────────────────────────────────────

    def closeEvent(self, event) -> None:  # type: ignore[override]
        """Stops the bridge before closing the window."""
        self._bridge.stop()
        super().closeEvent(event)
