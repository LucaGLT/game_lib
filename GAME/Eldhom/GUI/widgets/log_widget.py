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
        self._last_displayed_time: int = -1  # last mission_time shown via on_next_actor

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
        self._last_displayed_time = -1

    # ── Event handlers ─────────────────────────────────────────────────────────

    def on_any_event(self, msg: dict) -> None:
        """Formats and logs any engine event."""
        type_id  = msg.get("typeId", "")
        data     = _extract_data(msg)
        actor_id = data.get("actor_id", "")
        payload  = data.get("payload", "")

        # Enriched attack event: payload is a dict with target/damage/type.
        if type_id == "eldhom.pg.attacked" and isinstance(payload, dict):
            target    = str(payload.get("target", "?"))
            damage    = int(payload.get("damage", 0))
            atk_type  = str(payload.get("type", "MELEE"))
            atk_range = int(payload.get("range", 0))
            attacker  = actor_id or "?"
            if atk_type == "RANGED" and atk_range > 0:
                text = f"{attacker} attacca {target}: \U0001f3f9 {atk_range}\u25b8 {damage}\u274c"
            else:
                text = f"{attacker} attacca {target}: \u2694 {damage}\u274c"
            self.append(text, "#e08080")
            return

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
    def on_next_actor(self, msg: dict) -> None:
        """Logs timeline progression: empty slots then the acting actor's turn.

        Shows messages like:
            \u231b Tempo 6: Nessun Attore
            \u231b Tempo 7: \u25b6 Gioca Thael
        """
        data           = _extract_data(msg)
        actor_timeline = int(data.get("actor_timeline", data.get("mission_time", 0)))
        actor_id       = str(data.get("actor_id", ""))
        actor_name     = str(data.get("actor_name", actor_id))
        kind           = str(data.get("kind", "HERO"))

        _MAX_EMPTY = 8  # safety cap
        prev = self._last_displayed_time
        if prev >= 0 and actor_timeline > prev + 1:
            empty_count = actor_timeline - prev - 1
            show_count  = min(empty_count, _MAX_EMPTY)
            for t in range(prev + 1, prev + 1 + show_count):
                self.append(f"\u231b Tempo {t}: Nessun Attore", "#506070")
            if empty_count > _MAX_EMPTY:
                self.append(
                    f"  \u2026 [{empty_count - _MAX_EMPTY} slot vuoti omessi]",
                    "#506070",
                )

        self._last_displayed_time = actor_timeline
        if kind == "HERO":
            color = "#a8c8e8"
            label = f"\u25b6 Gioca {actor_name}"
        else:
            color = "#e0a888"
            label = f"\u2605 Attiva {actor_name}"
        self.append(f"\u231b Tempo {actor_timeline}: {label}", color)

# ─────────────────────────────────────────────────────────────────────────────
# Event → human-readable text
# ─────────────────────────────────────────────────────────────────────────────

_EVENT_TEMPLATES: dict[str, tuple[str, str]] = {
    "eldhom.pg.played_card":    ("{actor} gioca {payload}", "#d4b07a"),
    "eldhom.pg.simple_action":  ("{actor} esegue azione semplice", "#c8c870"),
    "eldhom.pg.moved":          ("{actor} si sposta → {payload}", "#80c0e0"),
    "eldhom.pg.attacked":       ("{actor} attacca {payload}", "#e08080"),  # actor può essere un mostro
    "eldhom.pg.healed":         ("{actor} si cura (+{payload} PV)", "#80e080"),
    "eldhom.pg.ko":             ("{actor} è KO!", "#ff6060"),
    "eldhom.pg.turn_ended":     ("{actor} termina il turno", "#888888"),
    "eldhom.monster.damaged":   ("Mostro {actor} danneggiato", "#e07070"),
    "eldhom.monster.defeated":  ("Mostro {actor} eliminato!", "#ff9900"),
    "eldhom.monster.moved":     ("{actor} si sposta \u2192 {payload}", "#e0c080"),
    "eldhom.group.activated":   ("Gruppo {actor} \u2014 turno completato", "#c07070"),
    "eldhom.group.eliminated":  ("Gruppo {actor} ELIMINATO!", "#ff6600"),
    "eldhom.formation.changed": ("Formazione cambiata: {payload}", "#a070d0"),
    "eldhom.deck.reshuffled":   ("{actor}: mazzo rimescolato", "#7090a0"),
    "eldhom.mission.time_advanced": ("⌛ {actor} → Tempo {payload}", "#607080"),
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
