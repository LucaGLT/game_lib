"""Le Pergamene di Eldhom — log widget.

LogWidget shows the action log: engine events formatted as human-readable
narrative messages.
"""
from __future__ import annotations

import json

from PySide6.QtWidgets import (
    QFrame,
    QLabel,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCore import Qt


class LogWidget(QFrame):
    """Scrollable list of action log messages."""

    _MAX_ENTRIES = 200

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)

        outer = QVBoxLayout(self)
        outer.setSpacing(0)
        outer.setContentsMargins(0, 0, 0, 0)

        title = QLabel("LOG", self)
        title.setStyleSheet(
            "background:#111; color:#777; font-size:11px; padding:2px 4px;"
        )
        outer.addWidget(title)

        self._scroll = QScrollArea(self)
        self._scroll.setWidgetResizable(True)
        self._scroll.setFrameShape(QFrame.Shape.NoFrame)

        self._content = QWidget()
        self._log_layout = QVBoxLayout(self._content)
        self._log_layout.setSpacing(1)
        self._log_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        self._log_layout.setContentsMargins(4, 4, 4, 4)

        self._scroll.setWidget(self._content)
        outer.addWidget(self._scroll)

        self._entry_count = 0
        self._labels: list[QLabel] = []

        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setStyleSheet(
            "QFrame { background:#0e0e0e; } QScrollArea { background:#0e0e0e; }"
        )

    # ── Public API ─────────────────────────────────────────────────────────────

    def append(self, text: str, color: str = "#aaaaaa") -> None:
        """Appends a plain text entry to the log.

        Args:
            text:  Message text.
            color: CSS color string for this entry.
        """
        if self._entry_count >= self._MAX_ENTRIES:
            old = self._labels.pop(0)
            self._log_layout.removeWidget(old)
            old.deleteLater()
        else:
            self._entry_count += 1

        lbl = QLabel(text, self._content)
        lbl.setStyleSheet(f"color:{color}; font-size:11px; padding:1px 2px;")
        lbl.setWordWrap(True)
        self._log_layout.addWidget(lbl)
        self._labels.append(lbl)

        # Auto-scroll to bottom
        scrollbar = self._scroll.verticalScrollBar()
        if scrollbar:
            scrollbar.setValue(scrollbar.maximum())

    def clear(self) -> None:
        """Clears all log entries."""
        for lbl in self._labels:
            lbl.deleteLater()
        self._labels.clear()
        self._entry_count = 0

    # ── Event handlers ─────────────────────────────────────────────────────────

    def on_any_event(self, msg: dict) -> None:
        """Formats and logs any engine event."""
        type_id = msg.get("typeId", "")
        data = _extract_data(msg)
        actor_id = data.get("actor_id", "")
        payload  = data.get("payload", "")

        text, color = _format_event(type_id, actor_id, payload)
        if text:
            self.append(text, color)

    def on_action_result(self, msg: dict) -> None:
        """Shows action result feedback (error messages only)."""
        data = _extract_data(msg)
        if not data.get("ok", True):
            error = data.get("error", "Errore sconosciuto")
            self.append(f"⚠ {error}", "#e08060")

    def on_mission_victory(self, msg: dict) -> None:
        """Logs mission victory."""
        self.append("🏆 MISSIONE COMPLETATA — Vittoria!", "#60e060")

    def on_mission_defeat(self, msg: dict) -> None:
        """Logs mission defeat."""
        data = _extract_data(msg)
        payload = data.get("payload", "")
        self.append(f"💀 MISSIONE FALLITA — {payload}", "#e06060")


# ─────────────────────────────────────────────────────────────────────────────
# Event → human-readable text
# ─────────────────────────────────────────────────────────────────────────────

_EVENT_TEMPLATES: dict[str, tuple[str, str]] = {
    "eldhom.pg.played_card":    ("{actor} gioca {payload}", "#d4b07a"),
    "eldhom.pg.simple_action":  ("{actor} esegue azione semplice", "#c8c870"),
    "eldhom.pg.moved":          ("{actor} si sposta → {payload}", "#80c0e0"),
    "eldhom.pg.attacked":       ("{actor} attacca {payload}", "#e08080"),
    "eldhom.pg.healed":         ("{actor} si cura (+{payload} PV)", "#80e080"),
    "eldhom.pg.ko":             ("{actor} è KO!", "#ff6060"),
    "eldhom.pg.turn_ended":     ("{actor} termina il turno", "#888888"),
    "eldhom.monster.damaged":   ("Mostro {actor} danneggiato", "#e07070"),
    "eldhom.monster.defeated":  ("Mostro {actor} eliminato!", "#ff9900"),
    "eldhom.group.activated":   ("Gruppo {actor} si attiva", "#c07070"),
    "eldhom.group.eliminated":  ("Gruppo {actor} ELIMINATO!", "#ff6600"),
    "eldhom.formation.changed": ("Formazione cambiata: {payload}", "#a070d0"),
    "eldhom.deck.reshuffled":   ("{actor}: mazzo rimescolato", "#7090a0"),
    "eldhom.mission.time_advanced": ("⌛ Tempo: {payload}", "#607080"),
}


def _format_event(type_id: str, actor_id: str, payload: object) -> tuple[str, str]:
    template_pair = _EVENT_TEMPLATES.get(type_id)
    if not template_pair:
        return "", "#888"
    template, color = template_pair
    payload_str = str(payload) if not isinstance(payload, str) else payload
    text = template.format(actor=actor_id or "?", payload=payload_str)
    return text, color


def _extract_data(msg: dict) -> dict:
    raw = msg.get("headers", {}).get("data", "{}")
    if isinstance(raw, dict):
        return raw
    try:
        return json.loads(raw)
    except Exception:
        return {}
