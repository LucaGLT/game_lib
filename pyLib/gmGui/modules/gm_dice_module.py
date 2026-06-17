"""GmDiceModule — Standard and Custom dice roller.

Subscribes to gmAlea dice events and exposes a UI for rolling dice
interactively.
"""
from __future__ import annotations

import json as _json

from PySide6.QtCore import QPropertyAnimation, Qt
from PySide6.QtGui import QFont
from PySide6.QtWidgets import (
    QAbstractItemView,
    QComboBox,
    QGraphicsOpacityEffect,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QPushButton,
    QSpinBox,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

from .base_module import BaseModule

_MAX_HISTORY: int = 10


class GmDiceModule(BaseModule):
    """Provides Standard (NdF) and Custom (weighted-face) dice rolling UI.

    TypeIds: ``gmAlea.dice.roll_result``, ``gmAlea.dice.profiles_snapshot``.
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
            "gmAlea.dice.profiles_snapshot",
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        container = QWidget()
        vbox = QVBoxLayout(container)
        vbox.setContentsMargins(6, 6, 6, 6)
        vbox.setSpacing(6)

        # ── Mode selector ─────────────────────────────────────────────────────
        self._mode_combo: QComboBox = QComboBox()
        self._mode_combo.addItem("Standard")
        self._mode_combo.addItem("Custom")
        vbox.addWidget(self._mode_combo)

        # ── Stacked widget: Standard / Custom pages ───────────────────────────
        self._stack: QStackedWidget = QStackedWidget()
        vbox.addWidget(self._stack)

        # Standard page
        std_page = QWidget()
        std_row = QHBoxLayout(std_page)
        std_row.setContentsMargins(0, 0, 0, 0)
        std_row.addWidget(QLabel("Dadi:"))
        self._count_spin: QSpinBox = QSpinBox()
        self._count_spin.setRange(1, 20)
        self._count_spin.setValue(1)
        std_row.addWidget(self._count_spin)
        std_row.addWidget(QLabel("d"))
        self._faces_spin: QSpinBox = QSpinBox()
        self._faces_spin.setRange(2, 100)
        self._faces_spin.setValue(6)
        std_row.addWidget(self._faces_spin)
        std_row.addStretch()
        self._stack.addWidget(std_page)

        # Custom page
        custom_page = QWidget()
        custom_row = QHBoxLayout(custom_page)
        custom_row.setContentsMargins(0, 0, 0, 0)
        custom_row.addWidget(QLabel("Profilo:"))
        self._profile_combo: QComboBox = QComboBox()
        custom_row.addWidget(self._profile_combo)
        custom_row.addWidget(QLabel("n:"))
        self._custom_count_spin: QSpinBox = QSpinBox()
        self._custom_count_spin.setRange(1, 10)
        self._custom_count_spin.setValue(1)
        custom_row.addWidget(self._custom_count_spin)
        custom_row.addStretch()
        self._stack.addWidget(custom_page)

        self._mode_combo.currentIndexChanged.connect(self._stack.setCurrentIndex)

        # ── LANCIA button ─────────────────────────────────────────────────────
        self._roll_btn: QPushButton = QPushButton("🎲  LANCIA")
        roll_font = QFont()
        roll_font.setPointSize(12)
        roll_font.setBold(True)
        self._roll_btn.setFont(roll_font)
        vbox.addWidget(self._roll_btn)

        # ── Result area ───────────────────────────────────────────────────────
        result_box = QGroupBox("Risultato")
        result_vbox = QVBoxLayout(result_box)

        self._result_label: QLabel = QLabel("—")
        result_font = QFont()
        result_font.setPointSize(28)
        result_font.setBold(True)
        self._result_label.setFont(result_font)
        self._result_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        result_vbox.addWidget(self._result_label)

        self._detail_label: QLabel = QLabel("")
        self._detail_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        result_vbox.addWidget(self._detail_label)

        vbox.addWidget(result_box)

        # Opacity flash animation on result label.
        self._result_effect: QGraphicsOpacityEffect = QGraphicsOpacityEffect(
            self._result_label
        )
        self._result_effect.setOpacity(1.0)
        self._result_label.setGraphicsEffect(self._result_effect)
        self._result_anim: QPropertyAnimation = QPropertyAnimation(
            self._result_effect, b"opacity"
        )
        self._result_anim.setDuration(400)

        # ── History ───────────────────────────────────────────────────────────
        history_box = QGroupBox("Storico (ultimi 10)")
        history_vbox = QVBoxLayout(history_box)

        self._history_list: QListWidget = QListWidget()
        self._history_list.setEditTriggers(
            QAbstractItemView.EditTrigger.NoEditTriggers
        )
        history_vbox.addWidget(self._history_list)

        self._clear_btn: QPushButton = QPushButton("Clear storico")
        self._clear_btn.clicked.connect(self._history_list.clear)
        history_vbox.addWidget(self._clear_btn)

        vbox.addWidget(history_box)

        # ── Wire up LANCIA ────────────────────────────────────────────────────
        self._roll_btn.clicked.connect(self._on_roll_clicked)

        return container

    # ── Internal slots ────────────────────────────────────────────────────────

    def _on_roll_clicked(self) -> None:
        if self._mode_combo.currentIndex() == 0:
            n = self._count_spin.value()
            f = self._faces_spin.value()
            self.send_command("gmAlea.dice.roll_request", {"count": n, "faces": f})
        else:
            profile = self._profile_combo.currentText()
            n = self._custom_count_spin.value()
            self.send_command(
                "gmAlea.dice.roll_custom_request", {"profile": profile, "count": n}
            )

    # ── Envelope routing ──────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid = msg.get("typeId", "")
        raw = msg.get("headers", {}).get("data", "{}")
        try:
            data: dict = _json.loads(raw) if isinstance(raw, str) else raw
        except Exception:
            data = {}

        if tid == "gmAlea.dice.roll_result":
            dice: list[int] = [int(d) for d in data.get("dice", [])]
            total: int = int(data.get("total", 0))
            self._result_label.setText(str(total))
            self._detail_label.setText(" + ".join(str(d) for d in dice))

            # Opacity fade-in animation on result label.
            self._result_anim.stop()
            self._result_anim.setStartValue(0.3)
            self._result_anim.setEndValue(1.0)
            self._result_anim.start()

            # Insert at top of history, cap at MAX_HISTORY.
            detail_str = ", ".join(str(d) for d in dice)
            self._history_list.insertItem(0, f"{total}  [{detail_str}]")
            while self._history_list.count() > _MAX_HISTORY:
                self._history_list.takeItem(self._history_list.count() - 1)

        elif tid == "gmAlea.dice.profiles_snapshot":
            profiles: list[str] = [str(p) for p in data.get("profiles", [])]
            self._profile_combo.clear()
            for p in profiles:
                self._profile_combo.addItem(p)

