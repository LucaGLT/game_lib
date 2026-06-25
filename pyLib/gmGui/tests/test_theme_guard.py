"""Phase 5 tests: theme verification and visual regression guards.

Covers:
- Runtime theme switching across all mandated theme ids.
- Basic visual/state invariants for core modules and custom widgets under each theme.
- Static guard against forbidden visual literals in gmGui source files.
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path
from unittest.mock import patch

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from gmGui.main_window import MainWindow
from gmGui.modules.gm_actor_module import GmActorModule
from gmGui.modules.gm_comp_deck_module import GmCompDeckModule
from gmGui.modules.gm_dice_module import GmDiceModule
from gmGui.modules.gm_flow_module import GmFlowModule
from gmGui.modules.gm_map_module import GmMapModule
from gmGui.theme_manager import ThemeManager, resolve_active_theme_id
from gmGui.widgets.hp_bar import HpBar
from gmGui.widgets.map_scene import MapScene
from gmGui.widgets.timeline_scene import TimelineScene

_THEME_IDS: tuple[str, ...] = ("scroll", "stone", "dark_moon", "blood", "techno")


@pytest.fixture(scope="module")
def qapp() -> QApplication:
    app = QApplication.instance() or QApplication(sys.argv)
    yield app


def _gm_gui_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _visual_source_files() -> list[Path]:
    root = _gm_gui_root()
    files: list[Path] = []
    files.extend(sorted((root / "modules").glob("*.py")))
    files.extend(sorted((root / "widgets").glob("*.py")))
    files.append(root / "main_window.py")
    return [p for p in files if p.is_file()]


def test_theme_switching_runtime_covers_all_theme_ids(qapp: QApplication) -> None:
    """MainWindow and ThemeManager accept all 5 theme ids at runtime."""
    from gmGui.engine_bridge.receiver import EngineReceiver

    manager = ThemeManager(qapp)
    available_ids = [t.theme_id for t in manager.available_themes()]
    assert available_ids == list(_THEME_IDS)

    with patch.object(EngineReceiver, "start"):
        win = MainWindow()

    try:
        assert set(win._theme_actions.keys()) == set(_THEME_IDS)

        themes_by_id = {t.theme_id: t for t in manager.available_themes()}
        for theme_id in _THEME_IDS:
            win._set_theme(theme_id)

            assert resolve_active_theme_id(qapp) == theme_id
            assert qapp.property("gm_theme_id") == theme_id
            assert win._theme_actions[theme_id].isChecked()

            stylesheet = qapp.styleSheet()
            assert stylesheet != ""
            assert themes_by_id[theme_id].background in stylesheet
            assert themes_by_id[theme_id].accent in stylesheet
    finally:
        win.close()


def test_core_modules_build_under_all_themes(qapp: QApplication) -> None:
    """Every core module builds and processes a basic event under each theme."""
    manager = ThemeManager(qapp)

    for theme_id in _THEME_IDS:
        manager.apply_theme(theme_id)

        actor = GmActorModule()
        actor.widget()
        actor.on_envelope(
            {
                "typeId": "gmActor.snapshot",
                "data": {
                    "actors": [
                        {
                            "actor_id": "hero_1",
                            "name": "Hero",
                            "faction_id": "Players",
                            "current_hp": 10,
                            "max_hp": 10,
                            "life_state": "ALIVE",
                            "statuses": {},
                            "equipment": {},
                            "area_id": "",
                        }
                    ]
                },
            }
        )
        assert "hero_1" in actor._actor_items

        flow = GmFlowModule()
        flow.widget()
        flow.on_envelope(
            {
                "typeId": "gmFlow.session.started",
                "data": {"session_id": f"sess_{theme_id}"},
            }
        )
        assert flow._btn_pause.isEnabled()

        dice = GmDiceModule()
        dice.widget()
        dice.on_envelope(
            {
                "typeId": "gmAlea.dice.roll_result",
                "headers": {"data": '{"dice": [2, 3], "total": 5}'},
            }
        )
        assert dice._result_label.text() == "5"

        comp_deck = GmCompDeckModule()
        comp_deck.widget()
        assert comp_deck._deck_combo is not None

        gm_map = GmMapModule()
        gm_map.widget()
        gm_map.on_envelope(
            {
                "typeId": "gmMap.map.loaded",
                "headers": {
                    "data": '{"locations": [{"location_id": 1}, '
                    '{"location_id": 2}], "edges": [[1, 2]]}'
                },
            }
        )
        assert gm_map._map_scene.node_count() == 2


def test_custom_widgets_visual_state_invariants_under_all_themes(
    qapp: QApplication,
) -> None:
    """Custom-painted widgets keep expected state behavior under every theme."""
    manager = ThemeManager(qapp)

    for theme_id in _THEME_IDS:
        manager.apply_theme(theme_id)

        bar = HpBar()
        bar.set_hp(8, 10)
        assert bar.ratio() == 0.8
        assert bar.bar_color().isValid()

        scene = TimelineScene()
        scene.set_actors(
            [
                {"actor_id": "a", "timeline_position": 0, "label": "A"},
                {"actor_id": "b", "timeline_position": 2, "label": "B"},
            ]
        )
        scene.select_actor("b")
        scene.advance_time(3)
        assert scene._selected_id == "b"
        assert scene._current_time == 3
        assert "a" in scene._actor_rects
        assert "b" in scene._actor_rects

        map_scene = MapScene()
        map_scene.load_map(
            [
                {"location_id": 1, "metadata": {"terrain": "grass"}},
                {"location_id": 2, "metadata": {"terrain": "water"}},
            ],
            [(1, 2)],
        )
        map_scene.move_actor("hero", 1)
        map_scene.update_location(1, {"terrain": "rock", "items": ["key"]})
        assert map_scene.node_count() == 2
        assert map_scene.edge_count() == 1
        assert map_scene.marker_location("hero") == 1


def test_visual_source_guard_forbidden_patterns() -> None:
    """Guards against reintroducing forbidden visual literals in source files."""
    forbidden_patterns: list[tuple[str, re.Pattern[str]]] = [
        ("inline setStyleSheet", re.compile(r"\.setStyleSheet\s*\(")),
        ("QColor constructor", re.compile(r"\bQColor\s*\(")),
        ("hardcoded QFont in setFont", re.compile(r"setFont\s*\(\s*QFont\s*\(")),
        ("hex color literal", re.compile(r"#[0-9A-Fa-f]{6}\b")),
    ]

    violations: list[str] = []

    for file_path in _visual_source_files():
        content = file_path.read_text(encoding="utf-8")
        for line_number, line in enumerate(content.splitlines(), start=1):
            stripped = line.strip()
            if stripped.startswith("#"):
                continue
            for rule_name, pattern in forbidden_patterns:
                if pattern.search(line):
                    rel_path = file_path.as_posix().split("/gmGui/")[-1]
                    violations.append(
                        f"{rel_path}:{line_number}: {rule_name}: {stripped}"
                    )

    assert not violations, "\n".join(violations)
