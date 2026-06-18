"""Theme manager for gmGui global QSS styling.

Applies one of the five predefined themes from .github/specs/gui-theme.yml
to the QApplication instance.
"""
from __future__ import annotations

from dataclasses import dataclass

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QFont
from PySide6.QtWidgets import QApplication


@dataclass(frozen=True)
class ThemePalette:
    """Palette tokens used to build an application-wide stylesheet."""

    theme_id: str
    display_name: str
    background: str
    panel: str
    border: str
    accent: str
    text: str
    corner_radius_px: int


_THEMES: dict[str, ThemePalette] = {
    "scroll": ThemePalette(
        theme_id="scroll",
        display_name="Scroll",
        background="#E8DFC8",
        panel="#F1E8D0",
        border="#7B6444",
        accent="#A8873A",
        text="#2E2418",
        corner_radius_px=4,
    ),
    "stone": ThemePalette(
        theme_id="stone",
        display_name="Stone",
        background="#BDB8AF",
        panel="#D2CEC6",
        border="#59544D",
        accent="#7A7A6B",
        text="#1E1E1E",
        corner_radius_px=2,
    ),
    "dark_moon": ThemePalette(
        theme_id="dark_moon",
        display_name="Dark Moon",
        background="#1A1A1E",
        panel="#26262D",
        border="#54546A",
        accent="#A89CC8",
        text="#E5E5E5",
        corner_radius_px=6,
    ),
    "blood": ThemePalette(
        theme_id="blood",
        display_name="Blood",
        background="#140A0A",
        panel="#241111",
        border="#6B1515",
        accent="#B52A2A",
        text="#F2E6E6",
        corner_radius_px=8,
    ),
    "techno": ThemePalette(
        theme_id="techno",
        display_name="Techno",
        background="#08131E",
        panel="#10202D",
        border="#00C8FF",
        accent="#00E5FF",
        text="#D8F8FF",
        corner_radius_px=12,
    ),
}

_THEME_ID_PROPERTY: str = "gm_theme_id"


class ThemeManager:
    """Applies and tracks the active gmGui theme on QApplication."""

    def __init__(self, app: QApplication | None = None) -> None:
        self._app: QApplication | None = app
        self._current_theme_id: str | None = None

    def available_themes(self) -> list[ThemePalette]:
        """Returns all available themes in deterministic order."""
        ordered_ids: list[str] = ["scroll", "stone", "dark_moon", "blood", "techno"]
        return [_THEMES[theme_id] for theme_id in ordered_ids]

    def current_theme_id(self) -> str | None:
        """Returns currently applied theme id, or None if not applied yet."""
        return self._current_theme_id

    def apply_theme(self, theme_id: str) -> None:
        """Applies the selected theme to QApplication via a global stylesheet."""
        theme: ThemePalette | None = _THEMES.get(theme_id)
        if theme is None:
            raise ValueError(f"Unknown theme id: {theme_id}")

        app: QApplication = self._resolve_app()
        app.setProperty(_THEME_ID_PROPERTY, theme.theme_id)
        app.setStyleSheet(self._build_stylesheet(theme))
        self._current_theme_id = theme.theme_id

    def _resolve_app(self) -> QApplication:
        app: QApplication | None = self._app or QApplication.instance()
        if app is None:
            raise RuntimeError("QApplication instance is required before applying a theme")
        self._app = app
        return app

    @staticmethod
    def _build_stylesheet(theme: ThemePalette) -> str:
        radius: int = theme.corner_radius_px
        return f"""
QWidget {{
    background-color: {theme.background};
    color: {theme.text};
}}

QMainWindow, QDockWidget, QMenuBar, QMenu, QStatusBar {{
    background-color: {theme.background};
    color: {theme.text};
}}

QGroupBox, QFrame {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
}}

QFrame#actor_header_card {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
}}

QGroupBox::title {{
    subcontrol-origin: margin;
    left: 8px;
    padding: 0 4px;
    color: {theme.text};
}}

QLabel {{
    color: {theme.text};
}}

QLabel[text_role="title"] {{
    font-weight: 700;
}}

QLabel[text_role="subtitle"] {{
    font-weight: 600;
}}

QLabel[text_role="secondary"] {{
    color: {theme.border};
}}

QLabel[chip="true"] {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    padding: 2px 8px;
}}

QLabel#actor_detail_state {{
    font-weight: 700;
}}

QLabel#actor_detail_state[life_state="alive"] {{
    border: 2px solid {theme.accent};
}}

QLabel#actor_detail_state[life_state="dying"] {{
    border: 2px solid {theme.accent};
}}

QLabel#actor_detail_state[life_state="dead"] {{
    border: 1px solid {theme.border};
}}

QLabel[flow_badge="true"] {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    padding: 4px 8px;
    font-weight: 600;
}}

QLineEdit, QComboBox, QSpinBox, QListWidget, QTreeWidget, QTableView, QTextEdit, QPlainTextEdit {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    padding: 4px 8px;
}}

QListWidget#flow_event_log {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
}}

QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QListWidget:focus, QTreeWidget:focus, QTableView:focus {{
    border: 2px solid {theme.accent};
}}

QPushButton {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    padding: 4px 8px;
}}

QPushButton:hover {{
    border: 2px solid {theme.accent};
}}

QPushButton:pressed {{
    background-color: {theme.background};
}}

QPushButton:disabled {{
    color: {theme.border};
    border: 1px solid {theme.border};
}}

QPushButton[button_variant="primary"] {{
    background-color: {theme.accent};
    border: 2px solid {theme.accent};
    font-weight: 700;
}}

QPushButton[button_variant="secondary"] {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
}}

QLabel#dice_value_box {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
}}

QTabWidget::pane {{
    border: 1px solid {theme.border};
    border-radius: {radius}px;
}}

QTabBar::tab {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    border-bottom: none;
    border-top-left-radius: {radius}px;
    border-top-right-radius: {radius}px;
    padding: 4px 8px;
}}

QTabBar::tab:hover {{
    border-color: {theme.accent};
}}

QTabBar::tab:selected {{
    background-color: {theme.background};
    border-color: {theme.accent};
}}

QToolTip {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    padding: 4px 8px;
}}
""".strip()


def resolve_active_theme_id(app: QApplication | None = None) -> str:
    """Returns active theme id from QApplication property, defaulting to scroll."""
    qapp = app or QApplication.instance()
    if qapp is None:
        return "scroll"
    value = qapp.property(_THEME_ID_PROPERTY)
    if isinstance(value, str) and value in _THEMES:
        return value
    return "scroll"


def resolve_active_palette(app: QApplication | None = None) -> ThemePalette:
    """Returns the active ThemePalette for custom-painted widgets."""
    return _THEMES[resolve_active_theme_id(app)]


def resolve_semantic_color(name: str, app: QApplication | None = None) -> QColor:
    """Returns a semantic QColor token derived from the active theme."""
    palette = resolve_active_palette(app)

    if name == "text":
        return QColor(palette.text)
    if name == "background":
        return QColor(palette.background)
    if name == "panel":
        return QColor(palette.panel)
    if name == "border":
        return QColor(palette.border)
    if name == "accent":
        return QColor(palette.accent)

    # State tokens used by logic widgets and custom-painted scenes.
    if name == "state_success":
        return QColor(Qt.GlobalColor.green)
    if name == "state_warning":
        return QColor(Qt.GlobalColor.yellow)
    if name == "state_error":
        return QColor(Qt.GlobalColor.red)
    if name == "state_disabled":
        return QColor(Qt.GlobalColor.gray)
    if name == "state_active":
        return QColor(Qt.GlobalColor.yellow)

    # Fallback for unknown semantic token names.
    return QColor(palette.text)


def resolve_typography(role: str) -> tuple[int, int]:
    """Returns `(point_size, weight)` for a semantic typography role."""
    if role == "title":
        return (12, 700)
    if role == "subtitle":
        return (10, 600)
    if role == "secondary":
        return (9, 400)
    # body
    return (10, 400)


def build_typography_font(role: str) -> QFont:
    """Builds a QFont from semantic typography role tokens."""
    point_size, weight = resolve_typography(role)
    font = QFont()
    font.setPointSize(point_size)
    font.setBold(weight >= 600)
    return font
