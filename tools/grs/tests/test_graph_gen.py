"""
test_graph_gen.py — Test per graph_gen.py (Fase 6).

Esegui con:
    cd tools
    python -m pytest grs/tests/test_graph_gen.py -v
"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import pytest
from grs.parser import parse_source
from grs.graph_gen import GraphGen, grapho_rule, grapho_all, _nid


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

_FULL = """
@meta
game dungeon
ns   dungeon.turn
version 1.0.0
@end
@targets
Target_Hero :: ACTOR SELECTED_ACTOR required
Target_Self :: ACTOR SELF             required
Target_Enemy :: ACTOR SELECTED_ENEMY range ADJACENT_LOCATION required
@end
@conditions
C_HeroAlive    :: ACTOR_HP_AT_OR_ABOVE(input.hero_id, 1)
C_Destination  :: LOCATION_EXISTS(input.destination)
C_CanBaseMove  :: C_HeroAlive AND C_Destination
@end
@effects
E_MoveHero       :: MOVE_ACTOR(Target_Hero, input.destination)
E_MarkActionUsed :: APPLY_STATUS(Target_Self, action_used) [stop]
E_MoveLog        :: MANUAL_EFFECT(dungeon.turn.base_move) [optional]
@end
@statuses
action_used :: ONE_ONLY UNTIL_NEXT_TURN
    ON_APPLY  ADD_TAG(Target_Self, action_spent)
    ON_REMOVE REMOVE_TAG(Target_Self, action_spent)
@end
@rules
Base_Move [priority=100] ::
    IF C_CanBaseMove
    ON Target_Hero
    THEN E_MoveHero AND THEN E_MarkActionUsed AND THEN E_MoveLog?
@end
@triggers
T_BlockDoubleAction [priority=5] ::
    ON_EVENT ACTION_SUBMITTED
    IF ACTOR_HAS_STATUS(event.actor_id, action_used)
    THEN E_MoveLog?

T_TurnEndNotify [priority=999] ::
    ON_EVENT ACTION_COMPLETED
    IF ACTOR_HAS_STATUS(event.actor_id, action_used)
    THEN E_MoveLog?
@end
"""


def doc():
    return parse_source(_FULL)


# ---------------------------------------------------------------------------
# _nid helper
# ---------------------------------------------------------------------------

def test_nid_sanitizes_dots():
    assert "." not in _nid("EV", "dungeon.turn.started")


def test_nid_sanitizes_hyphens():
    assert "-" not in _nid("R", "My-Rule")


# ---------------------------------------------------------------------------
# generate_rule: struttura base
# ---------------------------------------------------------------------------

def test_generate_rule_contains_graph_td():
    out = GraphGen(doc()).generate_rule("Base_Move")
    assert "graph TD" in out


def test_generate_rule_contains_rule_node():
    out = GraphGen(doc()).generate_rule("Base_Move")
    assert "Base_Move" in out
    assert "priority = 100" in out


def test_generate_rule_contains_mermaid_fences():
    out = GraphGen(doc()).generate_rule("Base_Move")
    assert out.startswith("```mermaid")
    assert "```" in out


def test_generate_rule_contains_condition_hexagon():
    out = GraphGen(doc()).generate_rule("Base_Move")
    # La condizione C_CanBaseMove appare come nodo esagono
    assert "C_CanBaseMove" in out


def test_generate_rule_contains_and_subgraph():
    out = GraphGen(doc()).generate_rule("Base_Move")
    # C_CanBaseMove è AND → subgraph AND
    assert "subgraph" in out
    assert "AND" in out


def test_generate_rule_contains_effect_nodes():
    out = GraphGen(doc()).generate_rule("Base_Move")
    assert "E_MoveHero" in out
    assert "E_MarkActionUsed" in out
    assert "E_MoveLog" in out


def test_generate_rule_effect_label_with_type():
    out = GraphGen(doc()).generate_rule("Base_Move")
    # Label "E_MoveHero\nMOVE_ACTOR"
    assert "MOVE_ACTOR" in out


def test_generate_rule_effect_label_optional():
    out = GraphGen(doc()).generate_rule("Base_Move")
    # E_MoveLog è optional → " · opt"
    assert "· opt" in out


def test_generate_rule_contains_status_node():
    out = GraphGen(doc()).generate_rule("Base_Move")
    # APPLY_STATUS(action_used) → nodo status arancio
    assert "action_used" in out
    assert "ONE_ONLY" in out


def test_generate_rule_contains_target_node():
    out = GraphGen(doc()).generate_rule("Base_Move")
    # Target_Hero come cerchio
    assert "Target_Hero" in out
    assert "SELECTED_ACTOR" in out


# ---------------------------------------------------------------------------
# Trigger connessi
# ---------------------------------------------------------------------------

def test_generate_rule_contains_trigger_action_submitted():
    out = GraphGen(doc()).generate_rule("Base_Move")
    assert "T_BlockDoubleAction" in out
    assert "ACTION_SUBMITTED" in out


def test_generate_rule_contains_trigger_action_completed():
    out = GraphGen(doc()).generate_rule("Base_Move")
    assert "T_TurnEndNotify" in out
    assert "ACTION_COMPLETED" in out


def test_trigger_connection_action_submitted_from_rule():
    """ACTION_SUBMITTED: freccia tratteggiata dal nodo regola."""
    out = GraphGen(doc()).generate_rule("Base_Move")
    rule_id = _nid("R", "Base_Move")
    ev_id   = _nid("EV", "T_BlockDoubleAction")
    assert f"{rule_id} -.-> {ev_id}" in out


def test_trigger_connection_action_completed_from_last_eff():
    """ACTION_COMPLETED: freccia tratteggiata dall'ultimo effetto."""
    out = GraphGen(doc()).generate_rule("Base_Move")
    # Ultimo effetto: indice 2 (E_MoveLog)
    last_eff_id = _nid("E", "Base_Move", "2")
    ev_id       = _nid("EV", "T_TurnEndNotify")
    assert f"{last_eff_id} -.-> {ev_id}" in out


# ---------------------------------------------------------------------------
# Stili
# ---------------------------------------------------------------------------

def test_styles_present():
    out = GraphGen(doc()).generate_rule("Base_Move")
    assert "fill:#CCE5FF" in out   # rule (blue)
    assert "fill:#FFF3CD" in out   # condition (yellow)
    assert "fill:#C8E6C9" in out   # effect (green)
    assert "fill:#FFE0B2" in out   # status (orange)
    assert "fill:#E1BEE7" in out   # target (purple)
    assert "fill:#FFCDD2" in out   # trigger (red)


# ---------------------------------------------------------------------------
# generate_all
# ---------------------------------------------------------------------------

def test_generate_all_contains_all_rules():
    out = GraphGen(doc()).generate_all()
    assert "## Base_Move" in out


def test_generate_all_has_mermaid_blocks():
    out = GraphGen(doc()).generate_all()
    assert out.count("```mermaid") >= 1


def test_grapho_all_has_header():
    out = grapho_all(doc())
    assert "GRS Rule Graphs" in out


# ---------------------------------------------------------------------------
# grapho_rule: not found
# ---------------------------------------------------------------------------

def test_grapho_rule_not_found():
    out = GraphGen(doc()).generate_rule("Nonexistent_Rule")
    assert "not found" in out


# ---------------------------------------------------------------------------
# Rule senza condizione
# ---------------------------------------------------------------------------

def test_rule_without_condition():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: ON T THEN E
@end
"""
    out = GraphGen(parse_source(src)).generate_rule("R")
    assert "graph TD" in out
    assert "E" in out


# ---------------------------------------------------------------------------
# Trigger non connesso non appare
# ---------------------------------------------------------------------------

def test_unrelated_trigger_not_shown():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
E_Log :: MANUAL_EFFECT(game.event) [optional]
@end
@rules
R :: ON T THEN E
@end
@triggers
T_Unrelated [priority=100] ::
    ON_EVENT LOCATION_ENTERED
    IF LOCATION_HAS_TAG(event.to_location, trap)
    THEN E_Log?
@end
"""
    out = GraphGen(parse_source(src)).generate_rule("R")
    # Nessun trigger su ACTION_SUBMITTED/COMPLETED e nessun status matching
    assert "T_Unrelated" not in out
