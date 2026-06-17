"""
test_parser.py — Unit test per parser.py

Esegui con:
    cd tools
    python -m pytest grs/tests/test_parser.py -v
"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import pytest
from grs.parser import parse_source, ParseError
from grs.ast_nodes import (
    CondAnd, CondOr, CondNot, CondAtom, CondRef,
    EffectCall,
)


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def grs(content: str):
    return parse_source(content)


# ---------------------------------------------------------------------------
# @meta
# ---------------------------------------------------------------------------

def test_meta_basic():
    doc = grs("""
@meta
game    checkers
ns      checkers.core
version 1.0.0
@end
""")
    assert doc.meta is not None
    assert doc.meta.game_id == "checkers"
    assert doc.meta.namespace == "checkers.core"
    assert doc.meta.version == "1.0.0"
    assert doc.meta.min_gmrules is None


def test_meta_with_min_gmrules():
    doc = grs("""
@meta
game test
ns   test.ns
version 0.1.0
min_gmrules 0.5.0
@end
""")
    assert doc.meta.min_gmrules == "0.5.0"


# ---------------------------------------------------------------------------
# @targets
# ---------------------------------------------------------------------------

def test_target_basic():
    doc = grs("""
@targets
Target_Self :: ACTOR SELF required
@end
""")
    assert len(doc.targets) == 1
    t = doc.targets[0]
    assert t.name == "Target_Self"
    assert t.kind == "ACTOR"
    assert t.selector == "SELF"
    assert t.required is True
    assert t.allow_self is True


def test_target_optional_no_self():
    doc = grs("""
@targets
Target_Ally :: ACTOR SELECTED_ALLY range SAME_LOCATION optional no_self
@end
""")
    t = doc.targets[0]
    assert t.required is False
    assert t.allow_self is False
    assert t.range_type == "SAME_LOCATION"


def test_target_range_within_n():
    doc = grs("""
@targets
T :: ACTOR SELECTED_ENEMY range WITHIN_N_LOCATIONS 3 required
@end
""")
    t = doc.targets[0]
    assert t.range_type == "WITHIN_N_LOCATIONS"
    assert t.range_n == 3


def test_target_multiple():
    doc = grs("""
@targets
T1 :: ACTOR SELF required
T2 :: CARD SELECTED_CARD required
@end
""")
    assert len(doc.targets) == 2
    assert doc.targets[1].kind == "CARD"


# ---------------------------------------------------------------------------
# @conditions
# ---------------------------------------------------------------------------

def test_condition_atomic():
    doc = grs("""
@conditions
C_Alive :: ACTOR_HP_AT_OR_ABOVE(input.hero_id, 1)
@end
""")
    c = doc.conditions[0]
    assert c.name == "C_Alive"
    assert isinstance(c.expr, CondAtom)
    assert c.expr.cond_type == "ACTOR_HP_AT_OR_ABOVE"
    assert c.expr.args == ["input.hero_id", "1"]


def test_condition_ref():
    doc = grs("""
@conditions
C_A :: ACTOR_EXISTS(input.actor_id)
C_B :: C_A
@end
""")
    assert isinstance(doc.conditions[1].expr, CondRef)
    assert doc.conditions[1].expr.name == "C_A"


def test_condition_and():
    doc = grs("""
@conditions
C_AB :: ACTOR_EXISTS(input.x) AND LOCATION_EXISTS(input.y)
@end
""")
    expr = doc.conditions[0].expr
    assert isinstance(expr, CondAnd)
    assert isinstance(expr.left, CondAtom)
    assert isinstance(expr.right, CondAtom)


def test_condition_or():
    doc = grs("""
@conditions
C_OR :: ACTOR_EXISTS(input.x) OR ACTOR_EXISTS(input.y)
@end
""")
    assert isinstance(doc.conditions[0].expr, CondOr)


def test_condition_not():
    doc = grs("""
@conditions
C_NOT :: NOT ACTOR_HAS_STATUS(input.hero_id, stunned)
@end
""")
    expr = doc.conditions[0].expr
    assert isinstance(expr, CondNot)
    assert isinstance(expr.operand, CondAtom)


def test_condition_complex():
    doc = grs("""
@conditions
C_Promo :: ACTOR_HAS_TAG(input.actor_id, white_piece) AND LOCATION_HAS_TAG(input.destination, promotion_white)
@end
""")
    expr = doc.conditions[0].expr
    assert isinstance(expr, CondAnd)


# ---------------------------------------------------------------------------
# @effects
# ---------------------------------------------------------------------------

def test_effect_move_actor():
    doc = grs("""
@effects
E_Move :: MOVE_ACTOR(Target_Piece, input.destination)
@end
""")
    e = doc.effects[0]
    assert e.name == "E_Move"
    assert e.call.effect_type == "MOVE_ACTOR"
    assert e.call.target_ref == "Target_Piece"
    assert e.call.args == ["input.destination"]


def test_effect_deal_damage():
    doc = grs("""
@effects
E_Dmg :: DEAL_DAMAGE(Target_Enemy, 2)
@end
""")
    e = doc.effects[0]
    assert e.call.effect_type == "DEAL_DAMAGE"
    assert e.call.args == ["2"]


def test_effect_modifier_optional():
    doc = grs("""
@effects
E_Log :: MANUAL_EFFECT(dungeon.turn.started) [optional]
@end
""")
    assert doc.effects[0].call.optional is True


def test_effect_modifier_stop():
    doc = grs("""
@effects
E_Cap :: APPLY_STATUS(Target_Enemy, captured) [stop]
@end
""")
    assert doc.effects[0].call.stop_on_failure is True


def test_effect_manual_no_target():
    doc = grs("""
@effects
E_Manual :: MANUAL_EFFECT(dungeon.turn.ended) [optional]
@end
""")
    e = doc.effects[0]
    assert e.call.target_ref is None
    assert e.call.args == ["dungeon.turn.ended"]


# ---------------------------------------------------------------------------
# @statuses
# ---------------------------------------------------------------------------

def test_status_basic():
    doc = grs("""
@statuses
action_used :: ONE_ONLY UNTIL_NEXT_TURN
    ON_APPLY ADD_TAG(Target_Self, action_spent)
    ON_REMOVE REMOVE_TAG(Target_Self, action_spent)
@end
""")
    s = doc.statuses[0]
    assert s.name == "action_used"
    assert s.stacking_mode == "ONE_ONLY"
    assert s.duration_type == "UNTIL_NEXT_TURN"
    assert len(s.hooks) == 2
    assert s.hooks[0].hook_type == "ON_APPLY"
    assert s.hooks[1].hook_type == "ON_REMOVE"


def test_status_for_n():
    doc = grs("""
@statuses
burning :: REFRESH FOR_N amount 2
    ON_TURN_END DEAL_DAMAGE(Target_Self, 1) [optional]
@end
""")
    s = doc.statuses[0]
    assert s.duration_type == "FOR_N"
    assert s.amount == 2


# ---------------------------------------------------------------------------
# @rules
# ---------------------------------------------------------------------------

def test_rule_basic():
    doc = grs("""
@conditions
C_CanMove :: LOCATION_EXISTS(input.destination)
@end
@targets
Target_Hero :: ACTOR SELECTED_ACTOR required
@end
@effects
E_Move :: MOVE_ACTOR(Target_Hero, input.destination)
E_Log  :: MANUAL_EFFECT(dungeon.move) [optional]
@end
@rules
Base_Move [priority=100] ::
    IF C_CanMove
    ON Target_Hero
    THEN E_Move AND THEN E_Log
@end
""")
    assert len(doc.rules) == 1
    r = doc.rules[0]
    assert r.name == "Base_Move"
    assert r.priority == 100
    assert r.enabled is True
    assert r.target == "Target_Hero"
    assert isinstance(r.condition, CondRef)
    assert len(r.effects) == 2


def test_rule_disabled():
    doc = grs("""
@rules
Old_Rule [disabled] ::
    ON Target_Hero
    THEN E_Move
@end
""")
    assert doc.rules[0].enabled is False


def test_rule_effect_optional_suffix():
    doc = grs("""
@rules
R :: ON Target_Hero THEN E_Move AND THEN E_Log?
@end
""")
    assert doc.rules[0].effects[1].optional is True


# ---------------------------------------------------------------------------
# @triggers
# ---------------------------------------------------------------------------

def test_trigger_basic():
    doc = grs("""
@triggers
T_Block [priority=5] ::
    ON_EVENT ACTION_SUBMITTED
    IF ACTOR_HAS_STATUS(event.actor_id, action_used)
    THEN E_ActionBlocked?
@end
""")
    tr = doc.triggers[0]
    assert tr.name == "T_Block"
    assert tr.priority == 5
    assert tr.event_type == "ACTION_SUBMITTED"
    assert isinstance(tr.condition, CondAtom)
    assert tr.effects[0].optional is True


def test_trigger_no_condition():
    doc = grs("""
@triggers
T_End [priority=999] ::
    ON_EVENT ACTION_COMPLETED
    THEN E_TurnEndLog?
@end
""")
    tr = doc.triggers[0]
    assert tr.condition is None
    assert len(tr.effects) == 1


# ---------------------------------------------------------------------------
# Errori di parsing
# ---------------------------------------------------------------------------

def test_missing_block_end():
    with pytest.raises(ParseError):
        parse_source("@meta\ngame test\n")


def test_unknown_block():
    with pytest.raises(ParseError):
        parse_source("@unknown\n@end\n")
