"""GmDiceModule — Standard and Custom dice roller.

Subscribes to gmAlea dice events and exposes a UI for rolling dice
interactively.
"""
from __future__ import annotations

import json as _json
import random as _random
from datetime import datetime

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
    QListWidgetItem,
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

    The module can be driven manually (the user picks the mode/dice and presses
    *LANCIA*) **or** entirely by the Core Engine through a single ad-hoc setup
    message — see :meth:`_apply_setup`.

    Subscribed typeIds
    ------------------
    - ``gmAlea.dice.setup``: engine-driven auto-configuration (mode, dice count,
      faces, custom profiles) and, optionally, the roll itself.
    - ``gmAlea.dice.roll_result``: a roll result to display.
    - ``gmAlea.dice.profiles_snapshot``: the list of available custom profiles.
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
            "gmAlea.dice.setup",
            "gmAlea.dice.roll_result",
            "gmAlea.dice.profiles_snapshot",
        ]

    # ── Widget construction ───────────────────────────────────────────────────

    def _build_widget(self) -> QWidget:
        container = QWidget()
        outer = QHBoxLayout(container)
        outer.setContentsMargins(6, 6, 6, 6)
        outer.setSpacing(8)

        # ══ Left column: configuration, roll, result ═════════════════════════
        left_widget = QWidget()
        vbox = QVBoxLayout(left_widget)
        vbox.setContentsMargins(0, 0, 0, 0)
        vbox.setSpacing(6)

        # ── Top bar with the history toggle ───────────────────────────────────
        top_bar = QHBoxLayout()
        top_bar.addStretch()
        self._toggle_history_btn: QPushButton = QPushButton("◄ Storico")
        self._toggle_history_btn.clicked.connect(self._toggle_history)
        top_bar.addWidget(self._toggle_history_btn)
        vbox.addLayout(top_bar)

        # ── Configuration group ───────────────────────────────────────────────
        config_box = QGroupBox("Configurazione")
        config_vbox = QVBoxLayout(config_box)

        self._mode_combo: QComboBox = QComboBox()
        self._mode_combo.addItem("Standard")
        self._mode_combo.addItem("Custom")
        config_vbox.addWidget(self._mode_combo)

        # Stacked widget: Standard / Custom pages
        self._stack: QStackedWidget = QStackedWidget()
        config_vbox.addWidget(self._stack)

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
        vbox.addWidget(config_box)

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

        self._sum_caption: QLabel = QLabel("SUM")
        sum_font = QFont()
        sum_font.setPointSize(9)
        sum_font.setBold(True)
        self._sum_caption.setFont(sum_font)
        self._sum_caption.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._sum_caption.setStyleSheet("color: #5b7cff;")
        result_vbox.addWidget(self._sum_caption)

        self._result_label: QLabel = QLabel("—")
        result_font = QFont()
        result_font.setPointSize(28)
        result_font.setBold(True)
        self._result_label.setFont(result_font)
        self._result_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        result_vbox.addWidget(self._result_label)

        # Row of individual dice boxes (repopulated on every roll).
        self._dice_row_widget: QWidget = QWidget()
        self._dice_row: QHBoxLayout = QHBoxLayout(self._dice_row_widget)
        self._dice_row.setContentsMargins(0, 0, 0, 0)
        self._dice_row.setAlignment(Qt.AlignmentFlag.AlignCenter)
        result_vbox.addWidget(self._dice_row_widget)

        # MIN / MAX / MEDIA stat row.
        stats_row = QHBoxLayout()
        self._min_label = self._add_stat_column(stats_row, "MIN")
        self._max_label = self._add_stat_column(stats_row, "MAX")
        self._avg_label = self._add_stat_column(stats_row, "MEDIA")
        result_vbox.addLayout(stats_row)

        vbox.addWidget(result_box)
        vbox.addStretch()

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

        outer.addWidget(left_widget, 1)

        # ══ Right column: collapsible history panel ══════════════════════════
        self._history_widget: QWidget = QWidget()
        right_vbox = QVBoxLayout(self._history_widget)
        right_vbox.setContentsMargins(0, 0, 0, 0)

        hist_header = QHBoxLayout()
        hist_title = QLabel("Storico")
        hist_title.setFont(sum_font)
        hist_header.addWidget(hist_title)
        hist_header.addStretch()
        hide_btn = QPushButton("Nascondi ►")
        hide_btn.clicked.connect(self._toggle_history)
        hist_header.addWidget(hide_btn)
        right_vbox.addLayout(hist_header)

        self._history_list: QListWidget = QListWidget()
        self._history_list.setEditTriggers(
            QAbstractItemView.EditTrigger.NoEditTriggers
        )
        self._history_list.setMinimumWidth(260)
        right_vbox.addWidget(self._history_list)

        self._clear_btn: QPushButton = QPushButton("🗑  Cancella storico")
        self._clear_btn.clicked.connect(self._history_list.clear)
        right_vbox.addWidget(self._clear_btn)

        outer.addWidget(self._history_widget, 1)

        # ── Wire up LANCIA ────────────────────────────────────────────────────
        self._roll_btn.clicked.connect(self._on_roll_clicked)

        return container

    # ── Layout helpers ────────────────────────────────────────────────────────

    def _add_stat_column(self, row: QHBoxLayout, caption: str) -> QLabel:
        """Adds a *caption / value* column to *row* and returns the value label."""
        column = QVBoxLayout()
        cap = QLabel(caption)
        cap.setAlignment(Qt.AlignmentFlag.AlignCenter)
        cap.setStyleSheet("color: #8a93a6; font-size: 9pt;")
        value = QLabel("—")
        value_font = QFont()
        value_font.setPointSize(13)
        value_font.setBold(True)
        value.setFont(value_font)
        value.setAlignment(Qt.AlignmentFlag.AlignCenter)
        column.addWidget(cap)
        column.addWidget(value)
        row.addLayout(column)
        return value

    def _make_die_box(self, value: int) -> QLabel:
        """Builds a small bordered box showing a single die value."""
        box = QLabel(str(value))
        box.setAlignment(Qt.AlignmentFlag.AlignCenter)
        box.setFixedSize(40, 40)
        box_font = QFont()
        box_font.setPointSize(13)
        box_font.setBold(True)
        box.setFont(box_font)
        box.setStyleSheet(
            "border: 1px solid #c8d0e0; border-radius: 8px; background: #ffffff;"
        )
        return box

    @staticmethod
    def _clear_layout(layout: QHBoxLayout) -> None:
        """Removes and deletes every widget currently held by *layout*."""
        while layout.count():
            item = layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()

    def _toggle_history(self) -> None:
        """Shows or hides the side history panel to widen/narrow the dialog."""
        visible = not self._history_widget.isVisible()
        self._history_widget.setVisible(visible)
        self._toggle_history_btn.setText("◄ Storico" if not visible else "Storico ►")

    def _spec_label(self, n_dice: int) -> str:
        """Returns the dice specification label (e.g. ``4d6`` or ``2× profilo``)."""
        if self._mode_combo.currentIndex() == 0:
            return f"{n_dice}d{self._faces_spin.value()}"
        profile = self._profile_combo.currentText()
        return f"{n_dice}× {profile}" if profile else f"{n_dice} dadi"

    def _make_history_entry(
        self,
        spec: str,
        dice: list[int],
        total: int,
        dmin: int,
        dmax: int,
        davg: int,
        timestamp: str,
    ) -> QWidget:
        """Builds a rich history row: spec, time, each die and SUM/MIN/MAX/MEDIA."""
        entry = QWidget()
        layout = QVBoxLayout(entry)
        layout.setContentsMargins(8, 6, 8, 6)
        layout.setSpacing(3)

        top = QHBoxLayout()
        spec_lbl = QLabel(spec)
        spec_font = QFont()
        spec_font.setBold(True)
        spec_lbl.setFont(spec_font)
        top.addWidget(spec_lbl)
        top.addStretch()
        time_lbl = QLabel(timestamp)
        time_lbl.setStyleSheet("color: #8a93a6; font-size: 8pt;")
        top.addWidget(time_lbl)
        layout.addLayout(top)

        dice_lbl = QLabel(", ".join(str(d) for d in dice))
        dice_lbl.setStyleSheet("color: #3a4253;")
        layout.addWidget(dice_lbl)

        stats = QLabel(
            f"<span style='color:#8a93a6'>SUM</span> <b>{total}</b>&nbsp;&nbsp;"
            f"<span style='color:#8a93a6'>MIN</span> <b>{dmin}</b>&nbsp;&nbsp;"
            f"<span style='color:#8a93a6'>MAX</span> <b>{dmax}</b>&nbsp;&nbsp;"
            f"<span style='color:#8a93a6'>MEDIA</span> <b>{davg}</b>"
        )
        layout.addWidget(stats)
        return entry

    # ── Internal slots ────────────────────────────────────────────────────────

    def _on_roll_clicked(self) -> None:
        """Performs a manual roll.

        In **Standard** mode the roll is computed locally and shown immediately,
        so the user can always roll on demand even when no engine dice service
        replies (e.g. in games where the dice outcome is not consumed). A
        ``gmAlea.dice.roll_request`` command is still emitted so an engine, if
        present, can observe the manual roll. In **Custom** mode (weighted faces)
        the roll is delegated to the engine, which owns the profile weights.
        """
        if self._mode_combo.currentIndex() == 0:
            n = self._count_spin.value()
            f = self._faces_spin.value()
            dice = [_random.randint(1, f) for _ in range(n)]
            self._show_result(dice, sum(dice))
            self.send_command("gmAlea.dice.roll_request", {"count": n, "faces": f})
        else:
            profile = self._profile_combo.currentText()
            n = self._custom_count_spin.value()
            self.send_command(
                "gmAlea.dice.roll_custom_request", {"profile": profile, "count": n}
            )

    # ── Engine-driven configuration ───────────────────────────────────────────

    def _set_controls_locked(self, locked: bool) -> None:
        """Enables/disables the manual configuration controls.

        When *locked* is true the user can no longer change the mode, the dice
        count/faces or the custom profile: the Core Engine drives configuration
        through ``gmAlea.dice.setup`` messages. The *LANCIA* button is **never**
        disabled, so the user can always roll manually even if the resulting roll
        is not used by the game.
        """
        for control in (
            self._mode_combo,
            self._count_spin,
            self._faces_spin,
            self._profile_combo,
            self._custom_count_spin,
        ):
            control.setEnabled(not locked)
        # LANCIA stays always enabled — manual rolls are allowed at any time.
        self._roll_btn.setEnabled(True)

    def _apply_setup(self, data: dict) -> None:
        """Auto-configures the widgets from an engine ``gmAlea.dice.setup`` message.

        Recognised payload fields (all optional)::

            {
                "mode":      "standard" | "custom",   # which page to show
                "count":     int,                     # number of dice
                "faces":     int,                     # faces per die (standard)
                "profiles":  [str],                   # fill the custom profile combo
                "profile":   str,                     # selected custom profile
                "locked":    bool,                    # disable manual controls
                "auto_roll": bool,                    # perform the roll immediately
                "result":    {"dice": [int], "total": int}  # show a precomputed roll
            }

        Rolling is driven by the same message: if ``result`` is present it is
        displayed directly (the engine already rolled); otherwise, when
        ``auto_roll`` is true, the standard roll-request flow is triggered.
        """
        # ── Custom profiles list ──────────────────────────────────────────────
        if "profiles" in data:
            profiles: list[str] = [str(p) for p in data.get("profiles", [])]
            self._profile_combo.clear()
            for profile in profiles:
                self._profile_combo.addItem(profile)

        # ── Mode (standard / custom) ──────────────────────────────────────────
        mode = str(data.get("mode", "")).lower()
        if mode == "standard":
            self._mode_combo.setCurrentIndex(0)
        elif mode == "custom":
            self._mode_combo.setCurrentIndex(1)

        # ── Dice count / faces ────────────────────────────────────────────────
        if "count" in data:
            count = int(data.get("count", 1))
            self._count_spin.setValue(count)
            self._custom_count_spin.setValue(count)
        if "faces" in data:
            self._faces_spin.setValue(int(data.get("faces", 6)))

        # ── Selected custom profile ───────────────────────────────────────────
        if "profile" in data:
            index = self._profile_combo.findText(str(data.get("profile", "")))
            if index >= 0:
                self._profile_combo.setCurrentIndex(index)

        # ── Lock manual controls when the engine owns the dice ────────────────
        self._set_controls_locked(bool(data.get("locked", False)))

        # ── Roll, driven by the same message ──────────────────────────────────
        result = data.get("result")
        if isinstance(result, dict):
            self._show_result(
                [int(d) for d in result.get("dice", [])],
                int(result.get("total", 0)),
            )
        elif bool(data.get("auto_roll", False)):
            self._on_roll_clicked()

    def _show_result(self, dice: list[int], total: int) -> None:
        """Displays a roll result with stats, dice boxes and a history entry."""
        self._result_label.setText(str(total))

        # Compute aggregate statistics (MEDIA is rounded down).
        if dice:
            dmin = min(dice)
            dmax = max(dice)
            davg = total // len(dice)
        else:
            dmin = dmax = davg = 0

        # Repopulate the individual dice boxes.
        self._clear_layout(self._dice_row)
        for die in dice:
            self._dice_row.addWidget(self._make_die_box(die))

        self._min_label.setText(str(dmin))
        self._max_label.setText(str(dmax))
        self._avg_label.setText(str(davg))

        # Opacity fade-in animation on result label.
        self._result_anim.stop()
        self._result_anim.setStartValue(0.3)
        self._result_anim.setEndValue(1.0)
        self._result_anim.start()

        # Insert a rich entry at the top of the history, capped at MAX_HISTORY.
        spec = self._spec_label(len(dice))
        timestamp = datetime.now().strftime("%H:%M:%S")
        entry = self._make_history_entry(spec, dice, total, dmin, dmax, davg, timestamp)
        item = QListWidgetItem()
        item.setSizeHint(entry.sizeHint())
        self._history_list.insertItem(0, item)
        self._history_list.setItemWidget(item, entry)
        while self._history_list.count() > _MAX_HISTORY:
            self._history_list.takeItem(self._history_list.count() - 1)

    # ── Envelope routing ──────────────────────────────────────────────────────

    def on_envelope(self, msg: dict) -> None:
        tid = msg.get("typeId", "")
        raw = msg.get("headers", {}).get("data", None)
        if raw is None:
            raw = msg.get("data", {})
        try:
            data: dict = _json.loads(raw) if isinstance(raw, str) else raw
        except Exception:
            data = {}

        if tid == "gmAlea.dice.setup":
            self._apply_setup(data)

        elif tid == "gmAlea.dice.roll_result":
            dice: list[int] = [int(d) for d in data.get("dice", [])]
            total: int = int(data.get("total", 0))
            self._show_result(dice, total)

        elif tid == "gmAlea.dice.profiles_snapshot":
            profiles: list[str] = [str(p) for p in data.get("profiles", [])]
            self._profile_combo.clear()
            for p in profiles:
                self._profile_combo.addItem(p)

