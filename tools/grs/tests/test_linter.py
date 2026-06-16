"""
test_linter.py — Test per linter.py (codici L-001..L-008).

Esegui con:
    cd tools
    python -m pytest grs/tests/test_linter.py -v
"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import pytest
from grs.parser import parse_source
from grs.linter import lint


def codes(source: str):
    """Parsa il sorgente e ritorna i codici diagnostici del linter."""
    doc = parse_source(source)
    return [d.code for d in lint(doc)]


def severities(source: str):
    doc = parse_source(source)
    return [(d.code, d.severity) for d in lint(doc)]


# ---------------------------------------------------------------------------
# Documento pulito
# ---------------------------------------------------------------------------

_CLEAN = """
@targets
T :: ACTOR SELF required
@end
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@statuses
s_ok :: ONE_ONLY UNTIL_NEXT_TURN
    ON_APPLY ADD_TAG(T, tag1)
@end
@rules
R :: ON T THEN E
@end
@triggers
Tr [priority=10] ::
    ON_EVENT ACTION_SUBMITTED
    THEN E?
@end
"""


def test_clean_no_errors():
    assert codes(_CLEAN) == []


# ---------------------------------------------------------------------------
# L-001 — Regola senza target
# ---------------------------------------------------------------------------

def test_l001_rule_missing_target():
    src = """
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
R :: THEN E
@end
"""
    # Il parser non supporta THEN senza ON direttamente; usiamo una regola
    # con target vuoto forziamo via un documento con rule.target = ""
    # usiamo il parser che accetta ON opzionale (il target rimane "")
    doc = parse_source(src)
    diag = lint(doc)
    assert any(d.code == "L-001" for d in diag)


# ---------------------------------------------------------------------------
# L-002 — Regola senza effetti
# ---------------------------------------------------------------------------

def test_l002_rule_missing_effects():
    # Forziamo via AST diretto: rule con lista effects vuota
    from grs.ast_nodes import GrsDocument, RuleDef
    doc = GrsDocument(meta=None)
    doc.rules.append(RuleDef(
        name="R_NoEffects",
        priority=0,
        enabled=True,
        condition=None,
        target="T",
        effects=[],
        line=1,
    ))
    diag = lint(doc)
    assert any(d.code == "L-002" for d in diag)


# ---------------------------------------------------------------------------
# L-003 — Trigger senza event_type
# ---------------------------------------------------------------------------

def test_l003_trigger_missing_event():
    from grs.ast_nodes import GrsDocument, TriggerDef, EffectEntry
    doc = GrsDocument(meta=None)
    doc.triggers.append(TriggerDef(
        name="T_NoEvent",
        priority=0,
        enabled=True,
        event_type="",      # mancante
        condition=None,
        effects=[EffectEntry(name="E", call=None, optional=False, line=1)],
        line=1,
    ))
    diag = lint(doc)
    assert any(d.code == "L-003" for d in diag)


# ---------------------------------------------------------------------------
# L-004 — Trigger senza effetti
# ---------------------------------------------------------------------------

def test_l004_trigger_missing_effects():
    from grs.ast_nodes import GrsDocument, TriggerDef
    doc = GrsDocument(meta=None)
    doc.triggers.append(TriggerDef(
        name="T_NoEffects",
        priority=0,
        enabled=True,
        event_type="ACTION_SUBMITTED",
        condition=None,
        effects=[],         # vuoto
        line=1,
    ))
    diag = lint(doc)
    assert any(d.code == "L-004" for d in diag)


# ---------------------------------------------------------------------------
# L-005 — Status: stacking mode invalido
# ---------------------------------------------------------------------------

def test_l005_bad_stacking():
    src = """
@statuses
s_bad :: INVALID_STACKING PERMANENT
    ON_APPLY ADD_TAG(T, x)
@end
"""
    assert "L-005" in codes(src)


def test_l005_valid_stacking_no_error():
    for mode in ["ONE_ONLY", "REFRESH", "ADD_STACK", "REPLACE", "UNIQUE_BY_SOURCE"]:
        src = f"""
@statuses
s :: {mode} PERMANENT
    ON_APPLY ADD_TAG(T, x)
@end
"""
        assert "L-005" not in codes(src)


# ---------------------------------------------------------------------------
# L-006 — Status: duration type invalido
# ---------------------------------------------------------------------------

def test_l006_bad_duration():
    src = """
@statuses
s_bad :: ONE_ONLY INVALID_DURATION
    ON_APPLY ADD_TAG(T, x)
@end
"""
    assert "L-006" in codes(src)


# ---------------------------------------------------------------------------
# L-007 — FOR_N senza amount
# ---------------------------------------------------------------------------

def test_l007_for_n_missing_amount():
    src = """
@statuses
s_burn :: REFRESH FOR_N
    ON_TURN_END DEAL_DAMAGE(T, 1) [optional]
@end
"""
    assert "L-007" in codes(src)


def test_l007_for_n_with_amount_ok():
    src = """
@statuses
s_burn :: REFRESH FOR_N amount 3
    ON_TURN_END DEAL_DAMAGE(T, 1) [optional]
@end
"""
    assert "L-007" not in codes(src)


# ---------------------------------------------------------------------------
# L-008 — WHILE_IN_LOCATION senza value
# ---------------------------------------------------------------------------

def test_l008_while_in_location_missing_value():
    src = """
@statuses
s_loc :: ONE_ONLY WHILE_IN_LOCATION
    ON_APPLY ADD_TAG(T, elevated)
@end
"""
    assert "L-008" in codes(src)


def test_l008_while_in_location_with_value_ok():
    src = """
@statuses
s_loc :: ONE_ONLY WHILE_IN_LOCATION value sq_15
    ON_APPLY ADD_TAG(T, elevated)
@end
"""
    assert "L-008" not in codes(src)


# ---------------------------------------------------------------------------
# Tutti gli errori sono ERROR (non WARNING)
# ---------------------------------------------------------------------------

def test_all_lint_errors_are_error_severity():
    from grs.ast_nodes import GrsDocument, StatusDef
    doc = GrsDocument(meta=None)
    doc.statuses.append(StatusDef(
        name="s", stacking_mode="BAD_MODE", duration_type="PERMANENT",
        amount=None, value=None, hooks=[], line=5,
    ))
    for d in lint(doc):
        assert d.severity == "ERROR"
