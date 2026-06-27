"""Le Pergamene di Eldhom — hero panel widget.

HeroPanelWidget shows the current state of a single PG:
- Name, class, HP bar, timeline position, formation position.
- Highlighted border when it is this hero's turn.
"""
from __future__ import annotations

from PySide6.QtWidgets import (
    QFrame,
    QLabel,
    QProgressBar,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt


class HeroPanelWidget(QFrame):
    """Compact panel displaying one PG's stats."""

    _ACTIVE_STYLE = (
        "QFrame { border: 2px solid #c8a060; border-radius:6px;"
        " background:#2a2510; padding:4px; }"
    )
    _INACTIVE_STYLE = (
        "QFrame { border: 1px solid #444; border-radius:6px;"
        " background:#1e1e1e; padding:4px; }"
    )

    def __init__(self, hero_id: str, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._hero_id  = hero_id
        self._is_active = False

        layout = QVBoxLayout(self)
        layout.setSpacing(3)
        layout.setContentsMargins(6, 6, 6, 6)

        self._name_label = QLabel("—", self)
        self._name_label.setStyleSheet("color:#c8a060; font-weight:bold; font-size:13px;")
        layout.addWidget(self._name_label)

        self._status_label = QLabel("", self)
        self._status_label.setStyleSheet("color:#999; font-size:11px;")
        layout.addWidget(self._status_label)

        self._hp_bar = QProgressBar(self)
        self._hp_bar.setRange(0, 6)
        self._hp_bar.setValue(6)
        self._hp_bar.setFormat("%v/%m ❤")
        self._hp_bar.setFixedHeight(16)
        self._hp_bar.setStyleSheet(
            "QProgressBar { border:1px solid #555; border-radius:3px; text-align:center;"
            " color:#eee; font-size:10px; background:#333; }"
            " QProgressBar::chunk { background:#c05050; }"
        )
        layout.addWidget(self._hp_bar)

        self._timeline_label = QLabel("⌛ 0", self)
        self._timeline_label.setStyleSheet("color:#8ab; font-size:11px;")
        layout.addWidget(self._timeline_label)

        self._pos_label = QLabel("📍 —", self)
        self._pos_label.setStyleSheet("color:#aaa; font-size:11px;")
        layout.addWidget(self._pos_label)

        self.setStyleSheet(self._INACTIVE_STYLE)
        self.setFixedWidth(170)

    @property
    def hero_id(self) -> str:
        """The hero actor ID this panel represents."""
        return self._hero_id

    def set_active(self, active: bool) -> None:
        """Highlights the panel border when it is this hero's turn."""
        self._is_active = active
        self.setStyleSheet(self._ACTIVE_STYLE if active else self._INACTIVE_STYLE)

    def update_from_dict(self, hero: dict) -> None:
        """Refreshes all displayed values from a hero state dict.

        Args:
            hero: Dict with keys id, name, class, hp, max_hp, timeline,
                  location, position, life_state.
        """
        name     = hero.get("name", self._hero_id)
        class_   = hero.get("class", "")
        hp       = hero.get("hp", 0)
        max_hp   = hero.get("max_hp", 6)
        timeline = hero.get("timeline", 0)
        location = hero.get("location", "—")
        position = hero.get("position", "FRONTLINE")
        state    = hero.get("life_state", 0)  # 0=ACTIVE, 1=KO, 2=DEAD

        self._name_label.setText(f"{name}  [{class_}]")
        self._hp_bar.setMaximum(max(max_hp, 1))
        self._hp_bar.setValue(max(0, hp))

        state_str = {0: "Attivo", 1: "KO", 2: "Morto"}.get(state, "Attivo")
        hand_limit = hero.get("hand_limit", 5)
        deck_count = hero.get("deck_count", 0)
        self._status_label.setText(
            f"{state_str}  •  mazzo:{deck_count}/{hand_limit}"
        )

        self._timeline_label.setText(f"⌛ {timeline}")
        pos_str = "Primo piano" if position == "FRONTLINE" else "Retro"
        self._pos_label.setText(f"📍 {location}  ({pos_str})")
