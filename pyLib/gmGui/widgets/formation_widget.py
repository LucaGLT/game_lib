"""FormationWidget — front-line / back-line formation display per faction.

Shows the tactical formation state for one location: for each faction a
two-column layout (Frontline / Backline) with numeric count badges.

When the backline count exceeds the frontline count, the faction row enters
an ``error`` visual state via ``setProperty`` + QSS — no hardcoded colours.

Data format (from ``gmactor.formation.updated``)::

    {
        "location_id": "stanza_1",
        "factions": [
            {"id": "heroes",   "label": "PG",     "frontline": 2, "backline": 1},
            {"id": "monsters", "label": "Mostri",  "frontline": 3, "backline": 0}
        ],
        "max_frontline": -1,
        "max_backline":  -1
    }

``max_frontline`` / ``max_backline`` are informational only; the widget uses
them to show a cap-exceeded warning state.  Pass ``-1`` for no cap.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)


class FormationWidget(QWidget):
    """Displays front/back rank formation for each faction in a location.

    Call :meth:`update_state` to refresh the display.
    The widget is read-only; it emits no signals.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._faction_rows: list[_FactionRow] = []
        self._build_layout()

    # ── Layout construction ────────────────────────────────────────────────────

    def _build_layout(self) -> None:
        root = QVBoxLayout()
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(8)

        # Header row: location label (updated in update_state) + column headers
        header_row = QHBoxLayout()
        header_row.setContentsMargins(0, 0, 0, 0)
        header_row.setSpacing(8)

        self._location_label = QLabel("Formazione")
        self._location_label.setProperty("text_role", "subtitle")
        header_row.addWidget(self._location_label)
        header_row.addStretch()

        fl_header = QLabel("Prima Linea")
        fl_header.setProperty("text_role", "secondary")
        fl_header.setAlignment(Qt.AlignmentFlag.AlignCenter)
        fl_header.setFixedWidth(80)
        header_row.addWidget(fl_header)

        bl_header = QLabel("Retroguardia")
        bl_header.setProperty("text_role", "secondary")
        bl_header.setAlignment(Qt.AlignmentFlag.AlignCenter)
        bl_header.setFixedWidth(80)
        header_row.addWidget(bl_header)

        root.addLayout(header_row)

        # Container for faction rows (populated in update_state)
        self._rows_container = QVBoxLayout()
        self._rows_container.setContentsMargins(0, 0, 0, 0)
        self._rows_container.setSpacing(4)
        root.addLayout(self._rows_container)

        root.addStretch()
        self.setLayout(root)

    # ── Public API ─────────────────────────────────────────────────────────────

    def update_state(self, data: dict) -> None:
        """Redraws the formation display from a state snapshot.

        Args:
            data: Dict with keys ``location_id`` (str), ``factions`` (list),
                  ``max_frontline`` (int), ``max_backline`` (int).
        """
        location_id:  str       = str(data.get("location_id", ""))
        factions:     list[dict] = data.get("factions", [])
        max_frontline: int       = int(data.get("max_frontline", -1))
        max_backline:  int       = int(data.get("max_backline", -1))

        if location_id:
            self._location_label.setText(f"Formazione — {location_id}")
        else:
            self._location_label.setText("Formazione")

        # Remove old rows from layout and list.
        for row in self._faction_rows:
            self._rows_container.removeWidget(row)
            row.deleteLater()
        self._faction_rows.clear()

        for faction in factions:
            row = _FactionRow(
                faction_id    = str(faction.get("id", "")),
                label         = str(faction.get("label", faction.get("id", "?"))),
                frontline     = int(faction.get("frontline", 0)),
                backline      = int(faction.get("backline", 0)),
                max_frontline = max_frontline,
                max_backline  = max_backline,
            )
            self._rows_container.addWidget(row)
            self._faction_rows.append(row)


class _FactionRow(QFrame):
    """One faction row: name label + frontline badge + backline badge."""

    def __init__(
        self,
        faction_id:    str,
        label:         str,
        frontline:     int,
        backline:      int,
        max_frontline: int,
        max_backline:  int,
        parent:        QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.setObjectName(f"faction_row_{faction_id}")

        # Determine state: error when backline > frontline.
        formation_illegal = backline > frontline
        fl_cap_exceeded   = max_frontline >= 0 and frontline > max_frontline
        bl_cap_exceeded   = max_backline  >= 0 and backline  > max_backline
        has_error = formation_illegal or fl_cap_exceeded or bl_cap_exceeded

        # Apply state via QSS property (no hardcoded colours).
        self.setProperty("formation_state", "error" if has_error else "normal")

        layout = QHBoxLayout()
        layout.setContentsMargins(8, 4, 8, 4)
        layout.setSpacing(8)

        name_lbl = QLabel(label)
        name_lbl.setProperty("text_role", "body")
        name_lbl.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred
        )
        layout.addWidget(name_lbl)

        fl_badge = _CountBadge(frontline, warning=fl_cap_exceeded)
        fl_badge.setFixedWidth(80)
        layout.addWidget(fl_badge)

        bl_badge = _CountBadge(backline, warning=formation_illegal or bl_cap_exceeded)
        bl_badge.setFixedWidth(80)
        layout.addWidget(bl_badge)

        self.setLayout(layout)


class _CountBadge(QLabel):
    """Centred numeric count badge, styled via QSS chip token."""

    def __init__(self, count: int, warning: bool = False, parent: QWidget | None = None) -> None:
        super().__init__(str(count), parent)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setProperty("chip", "true")
        # Drive error/normal styling via QSS property (no inline colours).
        self.setProperty("tone", "danger" if warning else "neutral")
