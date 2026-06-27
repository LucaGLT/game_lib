"""HpBar — colour-coded HP progress bar widget.

Draws a filled rectangle proportional to ``current_hp / max_hp`` and
triggers a brief opacity fade-in animation on each update.
"""
from __future__ import annotations

from PySide6.QtCore import QPropertyAnimation, QRect, Qt
from PySide6.QtGui import QColor, QPainter
from PySide6.QtWidgets import QGraphicsOpacityEffect, QWidget

from ..theme_manager import resolve_semantic_color


class HpBar(QWidget):
    """Draws a colour-coded HP bar with animated opacity change on update.

    Colour thresholds:
    - ``> 50 %`` of max_hp  → green
    - ``20–50 %`` of max_hp → yellow
    - ``< 20 %`` of max_hp  → red

    On each :meth:`set_hp` call, a 400 ms opacity animation (0.3 → 1.0)
    is triggered via :class:`QGraphicsOpacityEffect` so the bar flashes
    briefly to signal a value change.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._current_hp: int = 0
        self._max_hp: int = 1
        self.setMinimumHeight(24)
        self.setMinimumWidth(80)

        # QGraphicsOpacityEffect is the child-widget-safe way to animate opacity.
        # (windowOpacity only works for top-level windows.)
        self._effect: QGraphicsOpacityEffect = QGraphicsOpacityEffect(self)
        self._effect.setOpacity(1.0)
        self.setGraphicsEffect(self._effect)

        self._anim: QPropertyAnimation = QPropertyAnimation(self._effect, b"opacity")
        self._anim.setDuration(400)

    def set_hp(self, current: int, max_hp: int) -> None:
        """Updates the displayed HP values and triggers an opacity flash animation.

        Args:
            current: Current hit-point value.
            max_hp:  Maximum hit-point value (clamped to at least 1).
        """
        self._current_hp = current
        self._max_hp = max(max_hp, 1)
        self.update()
        self._anim.stop()
        self._anim.setStartValue(0.3)
        self._anim.setEndValue(1.0)
        self._anim.start()

    def ratio(self) -> float:
        """Returns ``current_hp / max_hp`` clamped to ``[0.0, 1.0]``."""
        return max(0.0, min(1.0, self._current_hp / self._max_hp))

    def bar_color(self) -> QColor:
        """Returns the fill colour determined by the current HP ratio."""
        r: float = self.ratio()
        if r > 0.5:
            return resolve_semantic_color("state_success_dark")
        if r >= 0.2:
            return resolve_semantic_color("state_warning")
        return resolve_semantic_color("state_error")

    def paintEvent(self, event) -> None:  # type: ignore[override]
        """Draws a background rect, a coloured fill and the HP text overlaid."""
        painter: QPainter = QPainter(self)
        painter.fillRect(self.rect(), resolve_semantic_color("border"))
        fill_width: int = int(self.width() * self.ratio())
        if fill_width > 0:
            painter.fillRect(QRect(0, 0, fill_width, self.height()), self.bar_color())
        # HP text centred on the bar — shadow pass for readability on any theme.
        text: str = f"{self._current_hp} / {self._max_hp}"
        shadow_rect = self.rect().translated(1, 1)
        painter.setPen(resolve_semantic_color("background"))
        painter.drawText(shadow_rect, Qt.AlignmentFlag.AlignCenter, text)
        painter.setPen(resolve_semantic_color("text"))
        painter.drawText(self.rect(), Qt.AlignmentFlag.AlignCenter, text)
        painter.end()
