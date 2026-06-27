"""Le Pergamene di Eldhom — hero panel widget.

HeroPanelWidget shows the current state of a single PG:
- Name, class, HP bar (gmGui.widgets.hp_bar.HpBar), timeline position, formation position.
- Highlighted border when it is this hero's turn.
"""
from __future__ import annotations

import sys
from pathlib import Path

from PySide6.QtWidgets import (
    QFrame,
    QLabel,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt

# Use the shared HpBar from pyLib when available; fall back to a QProgressBar otherwise.
_PYLIB_DIR = Path(__file__).resolve().parents[4] / "pyLib"
if str(_PYLIB_DIR) not in sys.path:
    sys.path.insert(0, str(_PYLIB_DIR))

try:
    from gmGui.widgets.hp_bar import HpBar as _HpBar
    _USE_GMHP = True
except ImportError:
    from PySide6.QtWidgets import QProgressBar  # type: ignore[assignment]
    _USE_GMHP = False


class HeroPanelWidget(QFrame):
    """Compact panel displaying one PG's stats."""

    _ACTIVE_STYLE = (
        "QFrame { border: 2px solid #A89CC8; border-radius:6px; padding:4px; }"
    )
    _INACTIVE_STYLE = (
        "QFrame { border: 1px solid #54546A; border-radius:6px; padding:4px; }"
    )

    def __init__(self, hero_id: str, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._hero_id  = hero_id
        self._is_active = False

        layout = QVBoxLayout(self)
        layout.setSpacing(3)
        layout.setContentsMargins(6, 6, 6, 6)

        self._name_label = QLabel("—", self)
        self._name_label.setStyleSheet("font-weight:bold; font-size:13px;")
        layout.addWidget(self._name_label)

        self._status_label = QLabel("", self)
        self._status_label.setStyleSheet("font-size:11px;")
        layout.addWidget(self._status_label)

        if _USE_GMHP:
            self._hp_bar: _HpBar = _HpBar(self)
            self._hp_bar.set_hp(6, 6)
        else:
            self._hp_bar = QProgressBar(self)  # type: ignore[assignment]
            self._hp_bar.setRange(0, 6)
            self._hp_bar.setValue(6)
            self._hp_bar.setFormat("%v/%m ❤")
            self._hp_bar.setFixedHeight(16)
        layout.addWidget(self._hp_bar)

        self._timeline_label = QLabel("⌛ 0", self)
        self._timeline_label.setStyleSheet("font-size:11px;")
        layout.addWidget(self._timeline_label)

        self._pos_label = QLabel("📍 —", self)
        self._pos_label.setStyleSheet("font-size:11px;")
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
        if _USE_GMHP:
            self._hp_bar.set_hp(max(0, hp), max(max_hp, 1))
        else:
            self._hp_bar.setMaximum(max(max_hp, 1))  # type: ignore[union-attr]
            self._hp_bar.setValue(max(0, hp))  # type: ignore[union-attr]

        state_str = {0: "Attivo", 1: "KO", 2: "Morto"}.get(state, "Attivo")
        hand_limit = hero.get("hand_limit", 5)
        deck_count = hero.get("deck_count", 0)
        self._status_label.setText(
            f"{state_str}  •  mazzo:{deck_count}/{hand_limit}"
        )

        self._timeline_label.setText(f"⌛ {timeline}")
        pos_str = "Primo piano" if position == "FRONTLINE" else "Retro"
        self._pos_label.setText(f"📍 {location}  ({pos_str})")
