"""HpBar — colour-coded HP progress bar widget.

Full implementation: Phase 5.
Phase 1 stub: stores values, no rendering.
"""
from __future__ import annotations

from PySide6.QtWidgets import QWidget


class HpBar(QWidget):
    """Draws a colour-coded HP bar with an animated opacity change on update.

    Colour thresholds:
    - ``> 50 %`` of max_hp  → green
    - ``20–50 %`` of max_hp → yellow
    - ``< 20 %`` of max_hp  → red

    Full implementation (Phase 5) adds:
    - ``paintEvent`` override with QPainter rectangle fill
    - ``QPropertyAnimation`` on the ``windowOpacity`` property when HP changes
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._current_hp: int = 0
        self._max_hp: int = 1

    def set_hp(self, current: int, max_hp: int) -> None:
        """Updates the displayed HP values.

        Args:
            current: Current hit-point value.
            max_hp:  Maximum hit-point value (must be > 0).

        Phase 1 stub: stores values only — no repaint.
        Full implementation: Phase 5.
        """
        # TODO: Phase 5 — update, repaint, and trigger QPropertyAnimation
        self._current_hp = current
        self._max_hp = max(max_hp, 1)
