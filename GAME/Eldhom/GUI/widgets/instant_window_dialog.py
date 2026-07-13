"""Le Pergamene di Eldhôm — instant-reaction window dialog.

InstantWindowDialog is a purely input-collecting modal opened when the
CoreEngine reports that one or more INSTANT cards may answer the action in
progress.  It lists every playable instant as a check-box row
(``<Actor> — <Card>``); the user ticks the ones to play (all, some or none)
and confirms.  No game logic lives here: the selected options are handed back
to the caller, which forwards them to the engine via ``eldhom.play_instants``.

All visual styling is applied exclusively through QSS — no hardcoded colour
values are present in this module.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QCheckBox,
    QDialog,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)


class InstantWindowDialog(QDialog):
    """Modal check-box dialog for the instant-reaction window.

    Args:
        options: Engine-provided list of playable instants. Each entry is a
            dict with ``actor_id``, ``card_id`` and ``card_name``.
        actor_names: Optional ``actor_id → display name`` lookup.
        parent: Optional parent widget.
    """

    def __init__(
        self,
        options: list[dict],
        actor_names: dict[str, str] | None = None,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.setWindowTitle("Carte Istantanee")
        self.setModal(True)
        self.setProperty("dialog_role", "reaction")

        names = actor_names or {}
        self._checks: list[tuple[QCheckBox, dict]] = []

        root = QVBoxLayout(self)
        root.setContentsMargins(16, 16, 16, 16)
        root.setSpacing(16)

        header = QLabel(
            "Sono giocabili le seguenti Carte Istantanee.\n"
            "Seleziona quali giocare (anche nessuna):"
        )
        header.setProperty("text_role", "primary")
        header.setWordWrap(True)
        root.addWidget(header)

        for opt in options:
            actor_id = str(opt.get("actor_id", ""))
            card_name = str(opt.get("card_name", opt.get("card_id", "")))
            actor_label = names.get(actor_id, actor_id)
            cb = QCheckBox(f"{actor_label}  \u2014  {card_name}")
            cb.setProperty("checkbox_role", "instant")
            self._checks.append((cb, opt))
            root.addWidget(cb)

        buttons = QHBoxLayout()
        buttons.setSpacing(8)
        buttons.addStretch(1)

        none_btn = QPushButton("Nessuna")
        none_btn.setProperty("button_role", "secondary")
        none_btn.clicked.connect(self.reject)
        buttons.addWidget(none_btn)

        play_btn = QPushButton("Gioca selezionate")
        play_btn.setProperty("button_role", "primary")
        play_btn.setDefault(True)
        play_btn.clicked.connect(self.accept)
        buttons.addWidget(play_btn)

        root.addLayout(buttons)

    def selected_options(self) -> list[dict]:
        """Returns the engine option dicts whose check-box is ticked."""
        return [opt for cb, opt in self._checks if cb.isChecked()]
