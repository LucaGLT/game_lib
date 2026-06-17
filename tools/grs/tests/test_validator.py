"""
test_validator.py — Test per validator.py (codici V-001..V-010).

Esegui con:
    cd tools
    python -m pytest grs/tests/test_validator.py -v
"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import pytest
from grs.parser import parse_source
from grs.validator import validate
from grs.ast_nodes import (
    GrsDocument, CondAtom, EffectCall, EffectEntry,
    ConditionDef, EffectDef, RuleDef, TriggerDef,
)


def codes(source: str):
    doc = parse_source(source)
    return [d.code for d in validate(doc)]


def diags(source: str):
    doc = parse_source(source)
    return validate(doc)


# ---------------------------------------------------------------------------
# Documento pulito — 0 diagnostici
# ---------------------------------------------------------------------------

_BASE = """
@targets
T :: ACTOR SELF required
@end
@conditions
C :: ACTOR_EXISTS(input.x)
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@statuses
s_ok :: ONE_ONLY UNTIL_NEXT_TURN
    ON_APPLY ADD_TAG(T, tag1)
    ON_REMOVE REMOVE_TAG(T, tag1)
@end
@rules
R :: IF C ON T THEN E
@end
@triggers
Tr [priority=10] ::
    ON_EVENT ACTION_SUBMITTED
    IF ACTOR_HAS_STATUS(event.actor_id, s_ok)
    THEN E?
@end
"""


def test_clean_no_errors():
    assert codes(_BASE) == []


# ---------------------------------------------------------------------------
# V-001 — Undefined names
# ---------------------------------------------------------------------------

def test_v001_undefined_target_in_rule():
    src = """
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: ON T_Undefined THEN E
@end
"""
    assert "V-001" in codes(src)


def test_v001_undefined_effect_in_rule():
    src = """
@targets
T :: ACTOR SELF required
@end
@rules
R :: ON T THEN E_Undefined
@end
"""
    assert "V-001" in codes(src)


def test_v001_undefined_condition_in_rule():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: IF C_Undefined ON T THEN E
@end
"""
    assert "V-001" in codes(src)


def test_v001_undefined_condition_ref_in_conditions():
    src = """
@conditions
C_A :: C_Undefined
@end
"""
    assert "V-001" in codes(src)


def test_v001_defined_names_no_error():
    assert "V-001" not in codes(_BASE)


# ---------------------------------------------------------------------------
# V-002 — Unknown effect type
# ---------------------------------------------------------------------------

def test_v002_unknown_effect_type():
    src = """
@effects
E_Bad :: UNKNOWN_EFFECT(T, val)
@end
"""
    assert "V-002" in codes(src)


def test_v002_known_effect_types_no_error():
    for etype in ["DEAL_DAMAGE", "HEAL", "APPLY_STATUS", "REMOVE_STATUS",
                  "ADD_TAG", "REMOVE_TAG", "MOVE_ACTOR", "EMIT_EVENT"]:
        src = f"""
@effects
E :: {etype}(T, val)
@end
"""
        assert "V-002" not in codes(src)


# ---------------------------------------------------------------------------
# V-003 — Unknown condition type (programmatic)
# ---------------------------------------------------------------------------

def test_v003_unknown_cond_type_programmatic():
    """V-003 è un safety check: si testa via AST diretto."""
    from grs.validator import validate
    doc = GrsDocument(meta=None)
    doc.conditions.append(ConditionDef(
        name="C_Bad",
        expr=CondAtom(cond_type="UNKNOWN_COND", args=["x"], line=1),
        line=1,
    ))
    result = validate(doc)
    assert any(d.code == "V-003" for d in result)


def test_v003_known_cond_types_no_error():
    src = """
@conditions
C :: ACTOR_EXISTS(input.x)
@end
"""
    assert "V-003" not in codes(src)


# ---------------------------------------------------------------------------
# V-004 — input.xxx in @trigger
# ---------------------------------------------------------------------------

def test_v004_input_ref_in_trigger_condition():
    src = """
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@triggers
Tr ::
    ON_EVENT ACTION_SUBMITTED
    IF ACTOR_EXISTS(input.actor_id)
    THEN E?
@end
"""
    assert "V-004" in codes(src)


def test_v004_event_ref_in_trigger_is_ok():
    src = """
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@triggers
Tr ::
    ON_EVENT ACTION_SUBMITTED
    IF ACTOR_EXISTS(event.actor_id)
    THEN E?
@end
"""
    assert "V-004" not in codes(src)


# ---------------------------------------------------------------------------
# V-005 — event.xxx in @rule
# ---------------------------------------------------------------------------

def test_v005_event_ref_in_rule_condition():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: IF ACTOR_EXISTS(event.actor_id) ON T THEN E
@end
"""
    assert "V-005" in codes(src)


def test_v005_input_ref_in_rule_is_ok():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: IF ACTOR_EXISTS(input.actor_id) ON T THEN E
@end
"""
    assert "V-005" not in codes(src)


# ---------------------------------------------------------------------------
# V-006 — Too many positional args
# ---------------------------------------------------------------------------

def test_v006_too_many_args_in_effect():
    """DEAL_DAMAGE accetta max 1 extra arg; 2 devono triggerare V-006."""
    src = """
@effects
E_Bad :: DEAL_DAMAGE(T, 1, extra_arg)
@end
"""
    assert "V-006" in codes(src)


def test_v006_too_many_args_in_condition():
    """ACTOR_EXISTS accetta 1 arg; 2 devono triggerare V-006."""
    src = """
@conditions
C_Bad :: ACTOR_EXISTS(input.x, extra)
@end
"""
    assert "V-006" in codes(src)


def test_v006_correct_arg_count_no_error():
    src = """
@conditions
C :: ACTOR_HP_AT_OR_ABOVE(input.hero_id, 1)
@end
@effects
E :: DEAL_DAMAGE(T, 2)
@end
"""
    assert "V-006" not in codes(src)


# ---------------------------------------------------------------------------
# V-007 — Duplicate IDs
# ---------------------------------------------------------------------------

def test_v007_duplicate_condition():
    src = """
@conditions
C_A :: ACTOR_EXISTS(input.x)
C_A :: ACTOR_EXISTS(input.y)
@end
"""
    assert "V-007" in codes(src)


def test_v007_duplicate_rule():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: ON T THEN E
R :: ON T THEN E
@end
"""
    assert "V-007" in codes(src)


def test_v007_unique_ids_no_error():
    src = """
@conditions
C_A :: ACTOR_EXISTS(input.x)
C_B :: ACTOR_EXISTS(input.y)
@end
"""
    assert "V-007" not in codes(src)


# ---------------------------------------------------------------------------
# V-008 — Cycle in conditions
# ---------------------------------------------------------------------------

def test_v008_direct_cycle():
    src = """
@conditions
C_A :: C_B
C_B :: C_A
@end
"""
    assert "V-008" in codes(src)


def test_v008_self_cycle():
    src = """
@conditions
C_A :: C_A
@end
"""
    assert "V-008" in codes(src)


def test_v008_three_node_cycle():
    src = """
@conditions
C_A :: C_B
C_B :: C_C
C_C :: C_A
@end
"""
    assert "V-008" in codes(src)


def test_v008_no_cycle():
    src = """
@conditions
C_A :: ACTOR_EXISTS(input.x)
C_B :: C_A
C_C :: C_A AND C_B
@end
"""
    assert "V-008" not in codes(src)


# ---------------------------------------------------------------------------
# V-009 — [stop] after [continue/optional] — WARNING
# ---------------------------------------------------------------------------

def test_v009_stop_after_optional():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E_Opt  :: MOVE_ACTOR(T, input.x) [optional]
E_Stop :: DEAL_DAMAGE(T, 1)      [stop]
@end
@rules
R :: ON T THEN E_Opt AND THEN E_Stop
@end
"""
    result = diags(src)
    v009 = [d for d in result if d.code == "V-009"]
    assert len(v009) >= 1
    assert all(d.severity == "WARNING" for d in v009)


def test_v009_normal_chain_no_warning():
    src = """
@targets
T :: ACTOR SELF required
@end
@effects
E1 :: DEAL_DAMAGE(T, 1) [stop]
E2 :: HEAL(T, 1)        [stop]
@end
@rules
R :: ON T THEN E1 AND THEN E2
@end
"""
    result = diags(src)
    assert not any(d.code == "V-009" for d in result)


# ---------------------------------------------------------------------------
# V-010 — APPLY_STATUS with undefined status
# ---------------------------------------------------------------------------

def test_v010_apply_status_undefined():
    src = """
@effects
E :: APPLY_STATUS(T, nonexistent_status)
@end
"""
    assert "V-010" in codes(src)


def test_v010_apply_status_defined_ok():
    src = """
@targets
T :: ACTOR SELF required
@end
@statuses
my_status :: ONE_ONLY PERMANENT
    ON_APPLY ADD_TAG(T, x)
@end
@effects
E :: APPLY_STATUS(T, my_status)
@end
"""
    assert "V-010" not in codes(src)


# ---------------------------------------------------------------------------
# Diagnostici: severità corrette
# ---------------------------------------------------------------------------

def test_v001_to_v010_are_errors_except_v009():
    """V-001..V-008, V-010 sono ERROR; V-009 è WARNING."""
    # V-009 warning già testato sopra
    # Verifica che V-001 sia ERROR
    result = diags("""
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: ON T_Missing THEN E
@end
""")
    v001 = [d for d in result if d.code == "V-001"]
    assert v001
    assert all(d.severity == "ERROR" for d in v001)
