"""Le Pergamene di Eldhom — mission selection dialog.

MissionSelectDialog lists all ``mission_XX.json`` files found in the
``data_dir`` and lets the player choose one before starting.
"""
from __future__ import annotations

import json
import os
from pathlib import Path

from PySide6.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QVBoxLayout,
)
from PySide6.QtCore import Qt


class MissionSelectDialog(QDialog):
    """Modal dialog for mission selection.

    After ``exec()`` returns ``QDialog.Accepted``, read :attr:`selected_mission_id`
    for the chosen mission identifier.
    """

    def __init__(
        self,
        data_dir: str | Path,
        parent=None,
    ) -> None:
        super().__init__(parent)
        self.setWindowTitle("Scegli Missione — Le Pergamene di Eldhom")
        self.setMinimumSize(400, 300)
        self.selected_mission_id: str = ""

        layout = QVBoxLayout(self)

        header = QLabel("Missioni disponibili:", self)
        header.setStyleSheet("color:#c8a060; font-size:14px; font-weight:bold;")
        layout.addWidget(header)

        self._list = QListWidget(self)
        self._list.setStyleSheet(
            "QListWidget { background:#1a1a1a; border:1px solid #444; color:#ddd;"
            " font-size:12px; }"
            "QListWidget::item:selected { background:#3a2a10; color:#ffe080; }"
        )
        self._list.itemDoubleClicked.connect(self._on_double_click)
        layout.addWidget(self._list)

        self._desc_label = QLabel("", self)
        self._desc_label.setWordWrap(True)
        self._desc_label.setStyleSheet("color:#888; font-size:11px; padding:4px;")
        layout.addWidget(self._desc_label)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel,
            self,
        )
        buttons.accepted.connect(self._on_accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

        self._missions: list[dict] = _scan_missions(str(data_dir))
        self._populate()

        self._list.currentItemChanged.connect(self._on_selection_changed)

    # ── Internal ──────────────────────────────────────────────────────────────

    def _populate(self) -> None:
        self._list.clear()
        for mission in self._missions:
            item = QListWidgetItem(
                f"  {mission['title']}  —  {mission['mission_id']}"
            )
            item.setData(Qt.ItemDataRole.UserRole, mission["mission_id"])
            self._list.addItem(item)
        if self._list.count() > 0:
            self._list.setCurrentRow(0)

    def _on_selection_changed(
        self, current: QListWidgetItem, _previous: QListWidgetItem
    ) -> None:
        if current is None:
            self._desc_label.setText("")
            return
        mission_id = current.data(Qt.ItemDataRole.UserRole)
        for m in self._missions:
            if m["mission_id"] == mission_id:
                self._desc_label.setText(m.get("description", ""))
                return

    def _on_double_click(self, _item: QListWidgetItem) -> None:
        self._on_accept()

    def _on_accept(self) -> None:
        current = self._list.currentItem()
        if current is None:
            return
        self.selected_mission_id = current.data(Qt.ItemDataRole.UserRole)
        self.accept()


# ─────────────────────────────────────────────────────────────────────────────
# Data scanning helper
# ─────────────────────────────────────────────────────────────────────────────

def _scan_missions(data_dir: str) -> list[dict]:
    """Reads mission JSON files and returns a list of summary dicts."""
    result: list[dict] = []
    try:
        for fname in sorted(os.listdir(data_dir)):
            if not fname.startswith("mission_") or not fname.endswith(".json"):
                continue
            path = os.path.join(data_dir, fname)
            try:
                with open(path, encoding="utf-8") as f:
                    data = json.load(f)
                result.append({
                    "mission_id":  data.get("mission_id", fname[:-5]),
                    "title":       data.get("title", fname),
                    "description": data.get("description", ""),
                })
            except Exception:
                pass
    except FileNotFoundError:
        pass
    return result
