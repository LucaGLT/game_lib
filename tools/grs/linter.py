"""
linter.py — Controlli strutturali post-parse (Fase 3).

Verifica la coerenza interna di ogni nodo AST senza riferimenti
incrociati tra blocchi (quelli vanno in validator.py).

Codici diagnostici:
    L-001  RuleDef: clausola ON (target) mancante
    L-002  RuleDef: clausola THEN (effect chain) mancante
    L-003  TriggerDef: ON_EVENT mancante
    L-004  TriggerDef: clausola THEN (effect chain) mancante
    L-005  StatusDef: stacking_mode non nel vocabolario
    L-006  StatusDef: duration_type non nel vocabolario
    L-007  StatusDef: FOR_N senza attributo 'amount'
    L-008  StatusDef: WHILE_IN_LOCATION senza attributo 'value'
"""

from __future__ import annotations
from typing import List, Set

from .ast_nodes import GrsDocument, RuleDef, TriggerDef, StatusDef
from .diagnostic import Diagnostic


_VALID_STACKING: Set[str] = {
    "ONE_ONLY", "REFRESH", "ADD_STACK", "REPLACE", "UNIQUE_BY_SOURCE",
}

_VALID_DURATION: Set[str] = {
    "PERMANENT", "UNTIL_REMOVED", "FOR_N", "UNTIL_NEXT_TURN", "WHILE_IN_LOCATION",
}


def lint(doc: GrsDocument) -> List[Diagnostic]:
    """
    Esegue i controlli strutturali (L-xxx) sul documento già parsato.
    Restituisce la lista di Diagnostic ordinata per numero di riga.
    """
    diags: List[Diagnostic] = []

    for rule in doc.rules:
        _check_rule(rule, diags)

    for trig in doc.triggers:
        _check_trigger(trig, diags)

    for st in doc.statuses:
        _check_status(st, diags)

    return sorted(diags, key=lambda d: d.line)


# ---------------------------------------------------------------------------
# Controlli per nodo
# ---------------------------------------------------------------------------

def _check_rule(rule: RuleDef, diags: List[Diagnostic]) -> None:
    if not rule.target:
        diags.append(Diagnostic(
            "ERROR", rule.line, "L-001",
            f"regola '{rule.name}': clausola ON (target) mancante",
        ))
    if not rule.effects:
        diags.append(Diagnostic(
            "ERROR", rule.line, "L-002",
            f"regola '{rule.name}': clausola THEN (effect chain) mancante",
        ))


def _check_trigger(trig: TriggerDef, diags: List[Diagnostic]) -> None:
    if not trig.event_type:
        diags.append(Diagnostic(
            "ERROR", trig.line, "L-003",
            f"trigger '{trig.name}': ON_EVENT mancante",
        ))
    if not trig.effects:
        diags.append(Diagnostic(
            "ERROR", trig.line, "L-004",
            f"trigger '{trig.name}': clausola THEN (effect chain) mancante",
        ))


def _check_status(st: StatusDef, diags: List[Diagnostic]) -> None:
    if st.stacking_mode not in _VALID_STACKING:
        diags.append(Diagnostic(
            "ERROR", st.line, "L-005",
            f"status '{st.name}': stacking mode '{st.stacking_mode}' non valido"
            f" (validi: {', '.join(sorted(_VALID_STACKING))})",
        ))
    if st.duration_type not in _VALID_DURATION:
        diags.append(Diagnostic(
            "ERROR", st.line, "L-006",
            f"status '{st.name}': duration type '{st.duration_type}' non valido"
            f" (validi: {', '.join(sorted(_VALID_DURATION))})",
        ))
    if st.duration_type == "FOR_N" and st.amount is None:
        diags.append(Diagnostic(
            "ERROR", st.line, "L-007",
            f"status '{st.name}': FOR_N richiede l'attributo 'amount N'",
        ))
    if st.duration_type == "WHILE_IN_LOCATION" and st.value is None:
        diags.append(Diagnostic(
            "ERROR", st.line, "L-008",
            f"status '{st.name}': WHILE_IN_LOCATION richiede l'attributo 'value V'",
        ))
