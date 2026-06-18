"""BoardWidget — the 3x3 clickable Tic-Tac-Toe grid (figure region: *Body*).

Each cell is a ``QPushButton``. Clicking an enabled cell emits
``cell_clicked(row, col)`` with 1-based coordinates. The window turns that into
a ``gmTris.move`` command for the C++ engine.
"""
from __future__ import annotations

from PySide6.QtWidgets import QGridLayout, QPushButton, QSizePolicy, QWidget
from PySide6.QtCore import Signal

# Maps a WinRules line id (as emitted by the engine in ``gmRules.game_won``)
# to the three board cells that compose it, so the winning line can be
# highlighted.
_LINE_CELLS: dict[str, list[tuple[int, int]]] = {
    "row_1": [(1, 1), (1, 2), (1, 3)],
    "row_2": [(2, 1), (2, 2), (2, 3)],
    "row_3": [(3, 1), (3, 2), (3, 3)],
    "col_1": [(1, 1), (2, 1), (3, 1)],
    "col_2": [(1, 2), (2, 2), (3, 2)],
    "col_3": [(1, 3), (2, 3), (3, 3)],
    "diag_main": [(1, 1), (2, 2), (3, 3)],
    "diag_anti": [(1, 3), (2, 2), (3, 1)],
}

class BoardWidget(QWidget):
    """Renders the grid and reports clicks via :attr:`cell_clicked`."""

    cell_clicked: Signal = Signal(int, int)  # (row, col), both 1-based

    def __init__(self, size: int = 3, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setObjectName("tris_board_root")
        self._size: int = size
        self._buttons: dict[tuple[int, int], QPushButton] = {}

        grid = QGridLayout(self)
        grid.setSpacing(6)
        for row in range(1, size + 1):
            for col in range(1, size + 1):
                button = QPushButton("")
                button.setMinimumSize(96, 96)
                button.setSizePolicy(
                    QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding
                )
                button.setProperty("tris_cell", "true")
                button.setProperty("tris_cell_state", "normal")
                button.clicked.connect(
                    lambda _checked=False, r=row, c=col: self.cell_clicked.emit(r, c)
                )
                grid.addWidget(button, row - 1, col - 1)
                self._buttons[(row, col)] = button
                self._refresh_button_style(button)

    def set_cell(self, row: int, col: int, mark: str) -> None:
        """Sets the symbol shown in a cell (``"X"``, ``"O"`` or empty)."""
        button = self._buttons.get((row, col))
        if button is not None:
            button.setText(mark if mark in ("X", "O") else "")

    def reset(self) -> None:
        """Clears every cell and re-enables the whole grid."""
        for button in self._buttons.values():
            button.setText("")
            button.setEnabled(True)
            button.setProperty("tris_cell_state", "normal")
            self._refresh_button_style(button)

    def set_enabled(self, enabled: bool) -> None:
        """Enables or disables all cells (e.g. when the game is over)."""
        for button in self._buttons.values():
            button.setEnabled(enabled)

    def highlight_line(self, line_id: str) -> None:
        """Highlights the three cells of the winning line, if recognised."""
        for cell in _LINE_CELLS.get(line_id, []):
            button = self._buttons.get(cell)
            if button is not None:
                button.setProperty("tris_cell_state", "win")
                self._refresh_button_style(button)

    @staticmethod
    def _refresh_button_style(button: QPushButton) -> None:
        """Re-polishes a board cell after dynamic property updates."""
        style = button.style()
        if style is not None:
            style.unpolish(button)
            style.polish(button)
        button.update()
