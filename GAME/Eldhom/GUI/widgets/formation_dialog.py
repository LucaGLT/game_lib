"""Formation assignment dialog for Eldhôm.

Used for interactive Scompaginamento (formation violation resolution) and
Schieramento (deliberate formation choice), including DISRUPT_ENEMY_FORMATION
card effects and DODGE reactions that invalidate the enemy formation.

The player checks actors they want in *Retroguardia*.  Unchecked actors go to
*Prima Linea*.  The constraint Retroguardia ≤ Prima Linea is enforced: the
OK button is disabled while violated.
"""

from __future__ import annotations

from PySide6.QtCore    import Qt
from PySide6.QtWidgets import (
    QCheckBox,
    QDialog,
    QDialogButtonBox,
    QFrame,
    QLabel,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)


class FormationDialog(QDialog):
    """Modal dialog for assigning actors to FRONTLINE or BACKLINE.

    Args:
        location_id: Location identifier (displayed in the title).
        faction_id:  Faction being reorganised.
        source:      Trigger reason — ``"scompaginamento"``, ``"overflow"``,
                     or ``"disrupt"``.
        actors:      List of dicts with keys ``actor_id``, ``name``,
                     ``in_backline`` (bool).
        parent:      Optional parent widget.
    """

    _SOURCE_LABELS: dict[str, str] = {
        "scompaginamento": "\u26a0 Scompaginamento: la Prima Linea \u00e8 vuota.",
        "overflow":        "\u26a0 Formazione non valida: troppi in Retroguardia.",
        "disrupt":         "\u2694 Scompaginamento (da carta): riorganizza la formazione nemica.",
    }

    def __init__(
        self,
        location_id: str,
        faction_id:  str,
        source:      str,
        actors:      list[dict],
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self._actors  = actors or []
        self._checks: list[QCheckBox] = []

        self.setWindowTitle(f"Formazione \u2014 {faction_id} @ {location_id}")
        self.setModal(True)
        self.setMinimumWidth(340)

        layout = QVBoxLayout(self)
        layout.setSpacing(8)
        layout.setContentsMargins(16, 16, 16, 16)

        # Source description label
        src_text = self._SOURCE_LABELS.get(
            source,
            "Decidi la Formazione:",
        )
        lbl_src = QLabel(src_text, self)
        lbl_src.setWordWrap(True)
        layout.addWidget(lbl_src)

        lbl_hint = QLabel(
            "\u2611 spunta = Retroguardia     \u2610 deseleziona = Prima Linea\n"
            "(Retroguardia \u2264 Prima Linea)",
            self,
        )
        lbl_hint.setWordWrap(True)
        layout.addWidget(lbl_hint)

        sep = QFrame(self)
        sep.setFrameShape(QFrame.HLine)
        layout.addWidget(sep)

        # Scrollable actor list
        scroll  = QScrollArea(self)
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.NoFrame)
        inner        = QWidget()
        inner_layout = QVBoxLayout(inner)
        inner_layout.setSpacing(4)

        for actor in self._actors:
            name    = str(actor.get("name", actor.get("actor_id", "?")))
            in_back = bool(actor.get("in_backline", False))
            cb = QCheckBox(f"{name}  \u2192  Retroguardia", inner)
            cb.setChecked(in_back)
            cb.stateChanged.connect(self._validate)
            inner_layout.addWidget(cb)
            self._checks.append(cb)

        scroll.setWidget(inner)
        layout.addWidget(scroll)

        # Validation feedback label
        self._val_label = QLabel("", self)
        self._val_label.setProperty("text_role", "error")
        layout.addWidget(self._val_label)

        # OK-only button box (formation resolution is mandatory — no cancel)
        btns = QDialogButtonBox(QDialogButtonBox.Ok, Qt.Horizontal, self)
        btns.accepted.connect(self.accept)
        self._ok_btn = btns.button(QDialogButtonBox.Ok)
        layout.addWidget(btns)

        self._validate()

    # ── Validation ─────────────────────────────────────────────────────────────

    def _validate(self) -> None:
        """Enables/disables OK button based on the formation constraint."""
        backline  = sum(1 for cb in self._checks if cb.isChecked())
        frontline = len(self._checks) - backline
        valid = backline <= frontline
        self._ok_btn.setEnabled(valid)
        if not valid:
            self._val_label.setText(
                f"\u26a0 Retroguardia ({backline}) > Prima Linea ({frontline}): "
                "sposta qualcuno in Prima Linea."
            )
        else:
            self._val_label.setText("")

    # ── Result accessor ────────────────────────────────────────────────────────

    def backline_actor_ids(self) -> list[str]:
        """Returns the actor_ids of actors assigned to Retroguardia."""
        result: list[str] = []
        for i, cb in enumerate(self._checks):
            if cb.isChecked() and i < len(self._actors):
                result.append(str(self._actors[i].get("actor_id", "")))
        return result
