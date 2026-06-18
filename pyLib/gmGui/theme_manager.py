"""Theme manager for gmGui global QSS styling.

Applies one of the five predefined themes from .github/specs/gui-theme.yml
to the QApplication instance.
"""
from __future__ import annotations

from dataclasses import dataclass

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QFont, QPalette
from PySide6.QtWidgets import QApplication, QStyleFactory


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
        background="#F3E9D2",
        panel="#E9DBBB",
        border="#8A6A3F",
        accent="#B88A3D",
        text="#2F2115",
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
        """Applies the selected theme to QApplication via stylesheet + QPalette."""
        theme: ThemePalette | None = _THEMES.get(theme_id)
        if theme is None:
            raise ValueError(f"Unknown theme id: {theme_id}")

        app: QApplication = self._resolve_app()
        
        # Try to set Fusion style; fallback to available style on error.
        try:
            fusion_style = QStyleFactory.create("Fusion")
            if fusion_style is not None:
                app.setStyle(fusion_style)
        except Exception:
            pass
        
        # Create and apply a custom QPalette to forcefully override defaults.
        palette = self._build_palette(theme)
        app.setPalette(palette)
        
        # Apply the stylesheet for fine-grained control.
        app.setProperty(_THEME_ID_PROPERTY, theme.theme_id)
        app.setFont(build_typography_font("body"))
        app.setStyleSheet(self._build_stylesheet(theme))
        
        # Forcefully propagate palette and repolish ALL widgets in the entire widget tree.
        # This is critical on Windows where native styles may override parent palette.
        self._propagate_theme_to_all_widgets(app, palette)
        
        self._current_theme_id = theme.theme_id

    @staticmethod
    def _propagate_theme_to_all_widgets(app: QApplication, palette: QPalette) -> None:
        """Recursively apply palette and force repolish on all widgets."""
        for widget in app.allWidgets():
            # Apply palette directly to widget.
            widget.setPalette(palette)
            # Force repolish to pick up stylesheet changes.
            if widget.style() is not None:
                widget.style().unpolish(widget)
                widget.style().polish(widget)
            # Force immediate update to redraw with new palette.
            widget.update()


    @staticmethod
    def _build_palette(theme: ThemePalette) -> QPalette:
        """Builds a QPalette from theme tokens."""
        palette = QPalette()
        
        bg_color = QColor(theme.background)
        panel_color = QColor(theme.panel)
        text_color = QColor(theme.text)
        border_color = QColor(theme.border)
        accent_color = QColor(theme.accent)
        
        # Base colors
        palette.setColor(QPalette.ColorRole.Base, panel_color)
        palette.setColor(QPalette.ColorRole.AlternateBase, bg_color)
        palette.setColor(QPalette.ColorRole.Window, bg_color)
        palette.setColor(QPalette.ColorRole.WindowText, text_color)
        palette.setColor(QPalette.ColorRole.Text, text_color)
        palette.setColor(QPalette.ColorRole.Button, panel_color)
        palette.setColor(QPalette.ColorRole.ButtonText, text_color)
        palette.setColor(QPalette.ColorRole.BrightText, text_color)
        
        # Highlight and interaction
        palette.setColor(QPalette.ColorRole.Highlight, accent_color)
        palette.setColor(QPalette.ColorRole.HighlightedText, text_color)
        palette.setColor(QPalette.ColorRole.Link, accent_color)
        palette.setColor(QPalette.ColorRole.LinkVisited, accent_color)
        
        # Shadow/edge colors for 3D effects
        palette.setColor(QPalette.ColorRole.Mid, border_color)
        palette.setColor(QPalette.ColorRole.Shadow, border_color)
        palette.setColor(QPalette.ColorRole.Light, bg_color)
        palette.setColor(QPalette.ColorRole.Dark, text_color)
        palette.setColor(QPalette.ColorRole.Midlight, panel_color)
        
        # Tooltip
        palette.setColor(QPalette.ColorRole.ToolTipBase, panel_color)
        palette.setColor(QPalette.ColorRole.ToolTipText, text_color)
        
        # Disabled state
        disabled_text = QColor(border_color)
        palette.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, disabled_text)
        palette.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, disabled_text)
        palette.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, disabled_text)
        
        return palette

    def _resolve_app(self) -> QApplication:
        app: QApplication | None = self._app or QApplication.instance()
        if app is None:
            raise RuntimeError("QApplication instance is required before applying a theme")
        self._app = app
        return app

    @staticmethod
    def _build_stylesheet(theme: ThemePalette) -> str:
        radius: int = theme.corner_radius_px
        accent_color = QColor(theme.accent)
        tone_success = accent_color.lighter(125).name()
        tone_warning = accent_color.name()
        tone_danger = accent_color.darker(135).name()
        return f"""
QWidget {{
    background-color: {theme.background};
    color: {theme.text};
    font-family: "Georgia", "DejaVu Serif", "Noto Serif", "Times New Roman", serif;
    font-size: 10pt;
}}

QMainWindow, QDockWidget, QMenuBar, QMenu, QStatusBar {{
    background-color: {theme.background};
    color: {theme.text};
}}

QToolBar {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    spacing: 8px;
    padding: 4px;
}}

QToolBar QLabel {{
    color: {theme.text};
}}

QMenuBar::item {{
    background: transparent;
    color: {theme.text};
    padding: 4px 8px;
}}

QMenuBar::item:selected {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
}}

QMenu::item:selected {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
}}

QDockWidget::title {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    border-bottom: 1px solid {theme.border};
    padding: 4px 8px;
}}

QHeaderView::section {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    padding: 4px 8px;
}}

QSplitter::handle {{
    background-color: {theme.border};
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
    font-size: 16px;
    font-weight: 700;
}}

QLabel[text_role="subtitle"] {{
    font-size: 12px;
    font-weight: 600;
}}

QLabel[text_role="body"] {{
    font-size: 10px;
    font-weight: 400;
}}

QLabel[text_role="body_sm"] {{
    font-size: 9px;
    font-weight: 400;
}}

QLabel[text_role="caption"] {{
    font-size: 9px;
    font-weight: 600;
    color: {theme.border};
}}

QLabel[text_role="micro"] {{
    font-size: 8px;
    font-weight: 400;
    color: {theme.border};
}}

QLabel[text_role="metric"] {{
    font-size: 20px;
    font-weight: 700;
}}

QLabel[text_role="display"] {{
    font-size: 56px;
    font-weight: 700;
}}

QLabel[text_role="secondary"] {{
    font-size: 9px;
    font-weight: 400;
    color: {theme.border};
}}

QLabel[tone="neutral"] {{
    color: {theme.text};
}}

QLabel[tone="muted"] {{
    color: {theme.border};
}}

QLabel[tone="accent"] {{
    color: {theme.accent};
}}

QLabel[tone="success"] {{
    color: {tone_success};
}}

QLabel[tone="warning"] {{
    color: {tone_warning};
}}

QLabel[tone="danger"] {{
    color: {tone_danger};
}}

QLabel[tone="disabled"] {{
    color: {theme.border};
}}

QLabel[weight="bold"] {{
    font-weight: 700;
}}

QLabel[dice_plain="true"] {{
    background-color: transparent;
    border: none;
    padding: 0;
}}

QWidget[dice_plain="true"] {{
    background-color: transparent;
    border: none;
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

QLineEdit, QComboBox, QSpinBox, QListWidget, QTreeWidget, QTableView, QGraphicsView, QTextEdit, QPlainTextEdit {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    padding: 4px 8px;
}}

QAbstractItemView {{
    background-color: {theme.panel};
    alternate-background-color: {theme.background};
    color: {theme.text};
    border: 1px solid {theme.border};
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
    border: 2px solid {theme.border};
    border-radius: {radius}px;
    padding: 4px 8px;
    font-size: 10px;
    font-weight: 600;
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

QPushButton[toggle_icon="true"] {{
    min-width: 18px;
    max-width: 18px;
    min-height: 16px;
    max-height: 16px;
    padding: 0;
    border: 1px solid {theme.border};
    border-radius: 2px;
    font-size: 9px;
    font-weight: 700;
}}

QPushButton[tris_cell="true"] {{
    min-width: 96px;
    min-height: 96px;
    background-color: {theme.panel};
    color: {theme.text};
    border: 2px solid {theme.border};
    border-radius: 8px;
    font-size: 40px;
    font-weight: 700;
}}

QPushButton[tris_cell="true"][tris_cell_state="win"] {{
    background-color: {theme.accent};
    border: 3px solid {theme.accent};
}}

QLabel#tris_board_status {{
    padding: 8px;
}}

QLabel#tris_turn_header {{
    padding: 8px;
}}

QLabel[chip="true"][tris_status="winner"] {{
    border: 2px solid {theme.accent};
    font-weight: 700;
}}

QLabel[chip="true"][tris_status="active_turn"] {{
    border: 2px solid {theme.accent};
}}

QLabel[chip="true"][tris_status="draw"] {{
    border: 2px solid {theme.border};
}}

QLabel#tris_error_bar[severity="idle"] {{
    color: {theme.border};
    padding: 8px;
}}

QLabel#tris_error_bar[severity="error"] {{
    color: {tone_danger};
    font-weight: 700;
    padding: 8px;
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

/* Aggressive rules for all button types */
QToolButton {{
    background-color: {theme.panel};
    color: {theme.text};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    padding: 4px 8px;
}}

QToolButton:hover {{
    border: 2px solid {theme.accent};
}}

QToolButton:pressed {{
    background-color: {theme.background};
}}

/* Dial and slider */
QDial {{
    background-color: {theme.panel};
}}

QSlider::groove {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
}}

QSlider::handle {{
    background-color: {theme.accent};
    border: 1px solid {theme.border};
}}

/* Progress bar */
QProgressBar {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
    border-radius: {radius}px;
    text-align: center;
    color: {theme.text};
}}

QProgressBar::chunk {{
    background-color: {theme.accent};
}}

/* Combo box drop-down */
QComboBox::drop-down {{
    background-color: {theme.panel};
    border-left: 1px solid {theme.border};
}}

QComboBox::down-arrow {{
    image: url(none);
    width: 12px;
    height: 12px;
    background-color: {theme.border};
}}

/* Custom game widgets */
TimelineScene, TimelineBlock {{
    background-color: {theme.panel};
}}

MapScene {{
    background-color: {theme.panel};
}}

DicePanel, HpBar {{
    background-color: {theme.panel};
    border: 1px solid {theme.border};
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
