"""Turn-state widgets.

- :class:`TurnHeaderWidget` (figure region *Body_Header*: "Stato del Giocatore
  di Turno") shows whose turn it is or the final result.
- :class:`TurnFooterWidget` (figure region *Body_Footer*: "Stato dei Turni")
  shows a per-player status badge for both players.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QHBoxLayout, QLabel, QWidget

# Human-readable label for each engine status, ordered by display priority
# (a winner badge wins over an active-turn badge, etc.).
_STATUS_LABEL: dict[str, str] = {
    "WINNER": "Vincitore",
    "DRAW": "Pareggio",
    "ACTIVE_TURN": "Turno corrente",
}
_STATUS_PRIORITY: list[str] = ["WINNER", "DRAW", "ACTIVE_TURN"]


def _mark_of(actor_id: str) -> str:
    """Returns ``"X"`` or ``"O"`` from an actor id such as ``"Player_X"``."""
    return "X" if actor_id.endswith("X") else "O"


class TurnHeaderWidget(QLabel):
    """Single label describing the current turn or the final outcome."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet("font-size: 18px; font-weight: bold; padding: 6px;")
        self.set_waiting()

    def set_waiting(self) -> None:
        """Resets the header to the pre-match state."""
        self.setText("In attesa dell'inizio della partita…")

    def set_turn(self, mark: str) -> None:
        """Shows which player must move now."""
        self.setText(f"Turno di:  Player {mark}  ({mark})")

    def set_winner(self, mark: str) -> None:
        """Shows the winner."""
        self.setText(f"🏆  Ha vinto Player {mark}!")

    def set_draw(self) -> None:
        """Shows a draw result."""
        self.setText("Partita pareggiata.")


class TurnFooterWidget(QWidget):
    """Two badges (Player X / Player O) reflecting each player's status."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._statuses: dict[str, set[str]] = {}
        self._labels: dict[str, QLabel] = {}

        layout = QHBoxLayout(self)
        for mark in ("X", "O"):
            label = QLabel()
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setStyleSheet(
                "font-size: 14px; padding: 6px; border: 1px solid #888;"
                " border-radius: 4px;"
            )
            actor_id = f"Player_{mark}"
            self._labels[actor_id] = label
            self._statuses[actor_id] = set()
            layout.addWidget(label)
        self._render_all()

    def set_players(self, actors: list[dict]) -> None:
        """Initialises both badges from a ``gmActor.snapshot`` actor list."""
        for actor in actors:
            actor_id = actor.get("actor_id", "")
            if actor_id in self._statuses:
                self._statuses[actor_id] = set(actor.get("statuses", []))
        self._render_all()

    def add_status(self, actor_id: str, status: str) -> None:
        """Adds a status to a player and refreshes its badge."""
        if actor_id in self._statuses:
            self._statuses[actor_id].add(status)
            self._render(actor_id)

    def remove_status(self, actor_id: str, status: str) -> None:
        """Removes a status from a player and refreshes its badge."""
        if actor_id in self._statuses:
            self._statuses[actor_id].discard(status)
            self._render(actor_id)

    def reset(self) -> None:
        """Clears every player's status."""
        for actor_id in self._statuses:
            self._statuses[actor_id].clear()
        self._render_all()

    # ── Internal rendering ─────────────────────────────────────────────────────

    def _render_all(self) -> None:
        for actor_id in self._labels:
            self._render(actor_id)

    def _render(self, actor_id: str) -> None:
        mark = _mark_of(actor_id)
        badge = "in attesa"
        for status in _STATUS_PRIORITY:
            if status in self._statuses[actor_id]:
                badge = _STATUS_LABEL[status]
                break
        self._labels[actor_id].setText(f"Player {mark}: {badge}")
