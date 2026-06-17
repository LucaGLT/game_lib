"""GmDiceModule — Standard and Custom dice roller.

Subscribes to the dice roll-result event from gmAlea.
Full implementation: Phase 7.
Phase 1 stub: renders a placeholder QLabel.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel, QWidget

from .base_module import BaseModule


class GmDiceModule(BaseModule):
    """Provides Standard (NdF) and Custom (weighted-face) dice rolling UI.

    Layout (Phase 7):
    - QComboBox: Standard / Custom mode selector
    - Standard mode: QSpinBox count (1-20) + QSpinBox faces (2-100)
    - Custom mode:   QComboBox profile + QSpinBox count (1-10)
    - [LANCIA] button (prominent)
    - QLabel result (large font, animated opacity on new result)
    - QLabel detail: individual dice values (e.g. "3 + 5 + 2")
    - QListWidget history (last 10 rolls) + [Clear] button
    """

    @property
    def module_id(self) -> str:
        return "gm_dice"

    @property
    def title(self) -> str:
        return "Dice"

    @property
    def default_area(self) -> Qt.DockWidgetArea:
        return Qt.DockWidgetArea.BottomDockWidgetArea

    def subscribed_type_ids(self) -> list[str]:
        return [
            "gmAlea.dice.roll_result",
            "gmAlea.dice.profiles_snapshot",  # custom profiles list on connect
        ]

    def _build_widget(self) -> QWidget:
        label = QLabel("stub – GmDice\n(Phase 7)")
        label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        return label

    def on_envelope(self, msg: dict) -> None:
        # TODO: Phase 7 — handle roll_result (animate result label) and
        #                  profiles_snapshot (populate Custom QComboBox)
        pass
