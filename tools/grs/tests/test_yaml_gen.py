"""
test_yaml_gen.py — Test per yaml_gen.py (Fase 5).

Esegui con:
    cd tools
    python -m pytest grs/tests/test_yaml_gen.py -v
"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import pytest
from grs.parser import parse_source
from grs.yaml_gen import generate, _quote, _scalar


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def yaml_of(source: str) -> str:
    return generate(parse_source(source))


# ---------------------------------------------------------------------------
# _scalar / _quote
# ---------------------------------------------------------------------------

def test_scalar_bool():
    assert _scalar(True)  == "true"
    assert _scalar(False) == "false"


def test_scalar_int():
    assert _scalar(42) == "42"


def test_scalar_none():
    assert _scalar(None) == "null"


def test_quote_reserved():
    assert _quote("true")  == '"true"'
    assert _quote("false") == '"false"'
    assert _quote("null")  == '"null"'
    assert _quote("yes")   == '"yes"'
    assert _quote("on")    == '"on"'


def test_quote_semver():
    # Versione semver: non è un numero valido e non ha caratteri speciali
    # YAML la tratta come stringa plain senza quoting
    assert _quote("1.0.0") == "1.0.0"
    assert _quote("0.5.0") == "0.5.0"


def test_quote_plain_id():
    # Identificatori semplici: non quotati
    assert _quote("ACTOR_EXISTS")     == "ACTOR_EXISTS"
    assert _quote("Target_Hero")      == "Target_Hero"
    assert _quote("action_used")      == "action_used"


def test_quote_dotted_event():
    # Nomi con punto: validi come scalari plain YAML, non richiedono quoting
    assert _quote("dungeon.turn.started") == "dungeon.turn.started"
    assert _quote("game.event")           == "game.event"


# ---------------------------------------------------------------------------
# @meta
# ---------------------------------------------------------------------------

def test_meta_basic():
    y = yaml_of("""
@meta
game checkers
ns   checkers.core
version 1.0.0
@end
""")
    assert "meta:" in y
    assert "game_id: checkers" in y
    assert "namespace: checkers.core" in y
    assert "1.0.0" in y   # versione come scalare plain


def test_meta_compatibility():
    y = yaml_of("""
@meta
game test
ns   test.ns
version 0.1.0
min_gmrules 0.5.0
@end
""")
    assert "compatibility:" in y
    assert "gmRules_min:" in y
    assert "0.5.0" in y   # scalare plain


# ---------------------------------------------------------------------------
# @targets
# ---------------------------------------------------------------------------

def test_target_basic():
    y = yaml_of("""
@targets
Target_Self :: ACTOR SELF required
@end
""")
    assert "targets:" in y
    assert "id: Target_Self" in y
    assert "kind: ACTOR" in y
    assert "selector: SELF" in y
    assert "required: true" in y
    assert "allow_self: true" in y


def test_target_optional_no_self():
    y = yaml_of("""
@targets
T :: ACTOR SELECTED_ALLY range SAME_LOCATION optional no_self
@end
""")
    assert "required: false" in y
    assert "allow_self: false" in y
    assert "range_type: SAME_LOCATION" in y


# ---------------------------------------------------------------------------
# @conditions
# ---------------------------------------------------------------------------

def test_condition_atom():
    y = yaml_of("""
@conditions
C :: ACTOR_HP_AT_OR_ABOVE(input.hero_id, 1)
@end
""")
    assert "conditions:" in y
    assert "type: ACTOR_HP_AT_OR_ABOVE" in y
    assert "subject_id_ref: input.hero_id" in y
    assert "amount: 1" in y


def test_condition_and():
    y = yaml_of("""
@conditions
C :: ACTOR_EXISTS(input.x) AND LOCATION_EXISTS(input.y)
@end
""")
    assert "op: ALL_OF" in y
    assert "children:" in y
    assert "type: ACTOR_EXISTS" in y
    assert "type: LOCATION_EXISTS" in y


def test_condition_or():
    y = yaml_of("""
@conditions
C :: ACTOR_EXISTS(input.x) OR ACTOR_EXISTS(input.y)
@end
""")
    assert "op: ANY_OF" in y


def test_condition_not():
    y = yaml_of("""
@conditions
C :: NOT ACTOR_HAS_STATUS(input.hero_id, stunned)
@end
""")
    assert "op: NOT" in y
    assert "children:" in y
    assert "type: ACTOR_HAS_STATUS" in y
    assert "status_id: stunned" in y


def test_condition_ref():
    y = yaml_of("""
@conditions
C_A :: ACTOR_EXISTS(input.x)
C_B :: C_A
@end
""")
    assert "ref: C_A" in y


# ---------------------------------------------------------------------------
# @effects
# ---------------------------------------------------------------------------

def test_effect_move_actor():
    y = yaml_of("""
@effects
E :: MOVE_ACTOR(Target_Piece, input.destination)
@end
""")
    assert "type: MOVE_ACTOR" in y
    assert "target: Target_Piece" in y
    assert "value_ref: input.destination" in y


def test_effect_deal_damage():
    y = yaml_of("""
@effects
E :: DEAL_DAMAGE(Target_Enemy, 2)
@end
""")
    assert "type: DEAL_DAMAGE" in y
    assert "amount: 2" in y


def test_effect_apply_status():
    y = yaml_of("""
@effects
E :: APPLY_STATUS(Target_Self, poisoned) [stop]
@end
""")
    assert "type: APPLY_STATUS" in y
    assert "status_id: poisoned" in y
    assert "stop_on_failure: true" in y


def test_effect_manual():
    y = yaml_of("""
@effects
E :: MANUAL_EFFECT(dungeon.turn.ended) [stop]
@end
""")
    assert "type: MANUAL_EFFECT" in y
    assert "stop_on_failure: true" in y
    # dotted name è uno scalare plain valido YAML
    assert "dungeon.turn.ended" in y


# ---------------------------------------------------------------------------
# @statuses
# ---------------------------------------------------------------------------

def test_status_stacking_duration():
    y = yaml_of("""
@statuses
action_used :: ONE_ONLY UNTIL_NEXT_TURN
    ON_APPLY ADD_TAG(Target_Self, action_spent)
@end
""")
    assert "stacking_policy:" in y
    assert "mode: ONE_ONLY" in y
    assert "default_duration:" in y
    assert "type: UNTIL_NEXT_TURN" in y
    assert "on_apply:" in y


def test_status_for_n():
    y = yaml_of("""
@statuses
burning :: REFRESH FOR_N amount 2
    ON_TURN_END DEAL_DAMAGE(Target_Self, 1) [optional]
@end
""")
    assert "type: FOR_N" in y
    assert "amount: 2" in y
    assert "on_turn_end:" in y


# ---------------------------------------------------------------------------
# @rules
# ---------------------------------------------------------------------------

def test_rule_basic():
    y = yaml_of("""
@targets
T :: ACTOR SELF required
@end
@conditions
C :: ACTOR_EXISTS(input.x)
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R [priority=100] :: IF C ON T THEN E
@end
""")
    assert "rules:" in y
    assert "id: R" in y
    assert "priority: 100" in y
    assert "enabled: true" in y
    assert "target: T" in y
    assert "ref: C" in y
    assert "ref: E" in y


def test_rule_disabled():
    y = yaml_of("""
@rules
R [disabled] :: ON T THEN E
@end
""")
    assert "enabled: false" in y


def test_rule_effect_optional_suffix():
    y = yaml_of("""
@rules
R :: ON T THEN E?
@end
""")
    assert "optional: true" in y


# ---------------------------------------------------------------------------
# @triggers
# ---------------------------------------------------------------------------

def test_trigger_type_prefix():
    y = yaml_of("""
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@triggers
Tr [priority=5] ::
    ON_EVENT ACTION_SUBMITTED
    THEN E?
@end
""")
    assert "triggers:" in y
    assert "type: ON_ACTION_SUBMITTED" in y    # prefisso ON_
    assert "priority: 5" in y


def test_trigger_condition():
    y = yaml_of("""
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@triggers
Tr ::
    ON_EVENT ACTOR_DAMAGED
    IF ACTOR_HP_AT_OR_BELOW(event.actor_id, 3)
    THEN E?
@end
""")
    assert "type: ACTOR_HP_AT_OR_BELOW" in y
    assert "subject_id_ref: event.actor_id" in y
    assert "amount: 3" in y


# ---------------------------------------------------------------------------
# Struttura globale (sezioni presenti)
# ---------------------------------------------------------------------------

def test_all_sections_present():
    y = yaml_of("""
@meta
game test
ns   test.ns
version 1.0.0
@end
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
s :: ONE_ONLY PERMANENT
    ON_APPLY ADD_TAG(T, x)
@end
@rules
R :: IF C ON T THEN E
@end
@triggers
Tr ::
    ON_EVENT ACTION_SUBMITTED
    THEN E?
@end
""")
    for section in ["meta:", "targets:", "conditions:", "effects:",
                    "statuses:", "rules:", "triggers:"]:
        assert section in y, f"sezione mancante: {section}"
