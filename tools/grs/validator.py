"""
validator.py — Validazioni semantiche (Fase 4).

Implementa le 10 regole della spec GRS v0.3 sezione
"Regole di validazione semantica (lint)".

Codici diagnostici:
    V-001  Nome usato in @rules/@triggers/@conditions non definito
    V-002  EffectCall: tipo effetto non nel vocabolario EffectType
    V-003  CondAtom: tipo condizione non nel vocabolario ConditionType
    V-004  Ref input.xxx usato in un @trigger (deve essere event.xxx)
    V-005  Ref event.xxx usato in una @rule (deve essere input.xxx)
    V-006  Troppi argomenti posizionali per il tipo
    V-007  ID duplicato nella stessa categoria
    V-008  Ciclo diretto in condizioni composite
    V-009  [stop] dopo [continue/optional] nella stessa chain — WARNING
    V-010  Status referenziato in APPLY_STATUS non dichiarato in @statuses
"""

from __future__ import annotations
from typing import Dict, FrozenSet, Iterator, List, Optional, Set, Tuple

from .ast_nodes import (
    GrsDocument,
    CondAtom, CondRef, CondNot, CondAnd, CondOr, CondExpr,
    EffectCall, EffectEntry, EffectDef,
    RuleDef, TriggerDef, ConditionDef, StatusDef,
)
from .diagnostic import Diagnostic


# ---------------------------------------------------------------------------
# Vocabolari e tabelle arg-count
# ---------------------------------------------------------------------------

_KNOWN_EFFECT_TYPES: Set[str] = {
    "MOVE_ACTOR", "DEAL_DAMAGE", "HEAL", "APPLY_STATUS", "REMOVE_STATUS",
    "ADD_TAG", "REMOVE_TAG", "DRAW_CARDS", "MOVE_CARD_TO_ZONE",
    "EMIT_EVENT", "MANUAL_EFFECT",
}

_KNOWN_COND_TYPES: Set[str] = {
    "ACTOR_EXISTS", "ACTOR_HAS_TAG", "ACTOR_HAS_STATUS",
    "ACTOR_HP_AT_OR_BELOW", "ACTOR_HP_AT_OR_ABOVE", "ACTOR_IN_LOCATION",
    "LOCATION_EXISTS", "LOCATION_HAS_TAG", "LOCATION_IS_ADJACENT",
    "TARGET_EXISTS", "TARGET_HAS_TAG", "TARGET_HAS_STATUS",
    "DECK_HAS_AT_LEAST", "CARD_IN_ZONE", "ALWAYS", "NEVER",
    "RESOURCE_AT_LEAST",
}

# (min_args, max_args) per EffectCall.args, escluso target_ref
_EFFECT_ARG_COUNTS: Dict[str, Tuple[int, int]] = {
    "MOVE_ACTOR":        (1, 1),
    "DEAL_DAMAGE":       (1, 1),
    "HEAL":              (1, 1),
    "APPLY_STATUS":      (1, 1),
    "REMOVE_STATUS":     (1, 1),
    "ADD_TAG":           (1, 1),
    "REMOVE_TAG":        (1, 1),
    "DRAW_CARDS":        (1, 2),   # deck_ref [, amount]
    "MOVE_CARD_TO_ZONE": (1, 1),
    "EMIT_EVENT":        (1, 1),
    "MANUAL_EFFECT":     (1, 1),   # event_name (target_ref assente)
}

# numero esatto di argomenti per CondAtom.args
_COND_ARG_COUNTS: Dict[str, int] = {
    "ACTOR_EXISTS":         1,
    "ACTOR_HAS_TAG":        2,
    "ACTOR_HAS_STATUS":     2,
    "ACTOR_HP_AT_OR_BELOW": 2,
    "ACTOR_HP_AT_OR_ABOVE": 2,
    "ACTOR_IN_LOCATION":    2,
    "LOCATION_EXISTS":      1,
    "LOCATION_HAS_TAG":     2,
    "LOCATION_IS_ADJACENT": 2,
    "TARGET_EXISTS":        0,
    "TARGET_HAS_TAG":       1,
    "TARGET_HAS_STATUS":    1,
    "DECK_HAS_AT_LEAST":    2,
    "CARD_IN_ZONE":         2,
    "ALWAYS":               0,
    "NEVER":                0,
    "RESOURCE_AT_LEAST":    2,
}


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def validate(doc: GrsDocument) -> List[Diagnostic]:
    """Esegue tutte le validazioni semantiche V-001..V-010."""
    return _Validator(doc).run()


# ---------------------------------------------------------------------------
# Validator (classe interna)
# ---------------------------------------------------------------------------

class _Validator:

    def __init__(self, doc: GrsDocument) -> None:
        self._doc = doc
        self._target_names: Set[str] = {t.name for t in doc.targets}
        self._condition_names: Set[str] = {c.name for c in doc.conditions}
        self._effect_names: Set[str] = {e.name for e in doc.effects}
        self._status_names: Set[str] = {s.name for s in doc.statuses}
        self._effect_map: Dict[str, EffectDef] = {e.name: e for e in doc.effects}
        self._diags: List[Diagnostic] = []

    def run(self) -> List[Diagnostic]:
        self._diags = []
        self._check_v007_duplicates()
        self._check_v001_undefined_names()
        self._check_v002_effect_vocab()
        self._check_v003_cond_vocab()
        self._check_v004_input_in_trigger()
        self._check_v005_event_in_rule()
        self._check_v006_arg_counts()
        self._check_v008_condition_cycles()
        self._check_v009_stop_after_continue()
        self._check_v010_apply_status()
        return sorted(self._diags, key=lambda d: d.line)

    # -----------------------------------------------------------------------
    # V-001 — Undefined names
    # -----------------------------------------------------------------------

    def _check_v001_undefined_names(self) -> None:
        # @conditions: ogni CondRef deve essere in condition_names
        for cdef in self._doc.conditions:
            for ref in _iter_cond_refs(cdef.expr):
                if ref.name not in self._condition_names:
                    self._err(ref.line, "V-001",
                              f"condizione '{ref.name}' non definita in @conditions")

        # @rules: target, effect names, condition refs
        for rule in self._doc.rules:
            if rule.target and rule.target not in self._target_names:
                self._err(rule.line, "V-001",
                          f"regola '{rule.name}': target '{rule.target}'"
                          f" non definito in @targets")
            if rule.condition:
                for ref in _iter_cond_refs(rule.condition):
                    if ref.name not in self._condition_names:
                        self._err(ref.line, "V-001",
                                  f"regola '{rule.name}':"
                                  f" condizione '{ref.name}' non definita in @conditions")
            for entry in rule.effects:
                if entry.name and entry.name not in self._effect_names:
                    self._err(entry.line, "V-001",
                              f"regola '{rule.name}':"
                              f" effetto '{entry.name}' non definito in @effects")

        # @triggers: condition refs, effect names
        for trig in self._doc.triggers:
            if trig.condition:
                for ref in _iter_cond_refs(trig.condition):
                    if ref.name not in self._condition_names:
                        self._err(ref.line, "V-001",
                                  f"trigger '{trig.name}':"
                                  f" condizione '{ref.name}' non definita in @conditions")
            for entry in trig.effects:
                if entry.name and entry.name not in self._effect_names:
                    self._err(entry.line, "V-001",
                              f"trigger '{trig.name}':"
                              f" effetto '{entry.name}' non definito in @effects")

        # @statuses hooks: effect names
        for st in self._doc.statuses:
            for hook in st.hooks:
                for entry in hook.chain:
                    if entry.name and entry.name not in self._effect_names:
                        self._err(entry.line, "V-001",
                                  f"status '{st.name}' {hook.hook_type}:"
                                  f" effetto '{entry.name}' non definito in @effects")

    # -----------------------------------------------------------------------
    # V-002 — Unknown effect type
    # -----------------------------------------------------------------------

    def _check_v002_effect_vocab(self) -> None:
        for call in self._iter_all_effect_calls():
            if call.effect_type not in _KNOWN_EFFECT_TYPES:
                self._err(call.line, "V-002",
                          f"tipo effetto sconosciuto: '{call.effect_type}'")

    # -----------------------------------------------------------------------
    # V-003 — Unknown condition type (safety check)
    # -----------------------------------------------------------------------

    def _check_v003_cond_vocab(self) -> None:
        for atom in self._iter_all_cond_atoms():
            if atom.cond_type not in _KNOWN_COND_TYPES:
                self._err(atom.line, "V-003",
                          f"tipo condizione sconosciuto: '{atom.cond_type}'")

    # -----------------------------------------------------------------------
    # V-004 — input.xxx in @triggers
    # -----------------------------------------------------------------------

    def _check_v004_input_in_trigger(self) -> None:
        for trig in self._doc.triggers:
            if trig.condition:
                for s, line in _iter_strings_in_cond(trig.condition):
                    if s.startswith("input."):
                        self._err(line, "V-004",
                                  f"trigger '{trig.name}': ref '{s}'"
                                  f" non valido nei trigger — usare event.xxx")
            for call in _iter_inline_calls(trig.effects):
                for s in _call_all_args(call):
                    if s.startswith("input."):
                        self._err(call.line, "V-004",
                                  f"trigger '{trig.name}': ref '{s}'"
                                  f" non valido nei trigger — usare event.xxx")

    # -----------------------------------------------------------------------
    # V-005 — event.xxx in @rules
    # -----------------------------------------------------------------------

    def _check_v005_event_in_rule(self) -> None:
        for rule in self._doc.rules:
            if rule.condition:
                for s, line in _iter_strings_in_cond(rule.condition):
                    if s.startswith("event."):
                        self._err(line, "V-005",
                                  f"regola '{rule.name}': ref '{s}'"
                                  f" non valido nelle regole — usare input.xxx")
            for call in _iter_inline_calls(rule.effects):
                for s in _call_all_args(call):
                    if s.startswith("event."):
                        self._err(call.line, "V-005",
                                  f"regola '{rule.name}': ref '{s}'"
                                  f" non valido nelle regole — usare input.xxx")

    # -----------------------------------------------------------------------
    # V-006 — Too many positional args
    # -----------------------------------------------------------------------

    def _check_v006_arg_counts(self) -> None:
        for call in self._iter_all_effect_calls():
            if call.effect_type in _EFFECT_ARG_COUNTS:
                _, max_a = _EFFECT_ARG_COUNTS[call.effect_type]
                if len(call.args) > max_a:
                    self._err(call.line, "V-006",
                              f"effetto '{call.effect_type}':"
                              f" {len(call.args)} argomenti, max atteso {max_a}")

        for atom in self._iter_all_cond_atoms():
            if atom.cond_type in _COND_ARG_COUNTS:
                expected = _COND_ARG_COUNTS[atom.cond_type]
                if len(atom.args) > expected:
                    self._err(atom.line, "V-006",
                              f"condizione '{atom.cond_type}':"
                              f" {len(atom.args)} argomenti, max atteso {expected}")

    # -----------------------------------------------------------------------
    # V-007 — Duplicate IDs
    # -----------------------------------------------------------------------

    def _check_v007_duplicates(self) -> None:
        self._check_dup("@targets",    [(t.name, t.line) for t in self._doc.targets])
        self._check_dup("@conditions", [(c.name, c.line) for c in self._doc.conditions])
        self._check_dup("@effects",    [(e.name, e.line) for e in self._doc.effects])
        self._check_dup("@statuses",   [(s.name, s.line) for s in self._doc.statuses])
        self._check_dup("@rules",      [(r.name, r.line) for r in self._doc.rules])
        self._check_dup("@triggers",   [(t.name, t.line) for t in self._doc.triggers])

    def _check_dup(self, block: str, pairs: List[Tuple[str, int]]) -> None:
        seen: Dict[str, int] = {}
        for name, line in pairs:
            if name in seen:
                self._err(line, "V-007",
                          f"{block}: ID '{name}' duplicato"
                          f" (prima occorrenza riga {seen[name]})")
            else:
                seen[name] = line

    # -----------------------------------------------------------------------
    # V-008 — Cycles in @conditions
    # -----------------------------------------------------------------------

    def _check_v008_condition_cycles(self) -> None:
        # Grafo: cond_name → lista di nomi CondRef direttamente referenziati
        graph: Dict[str, List[str]] = {
            c.name: [r.name for r in _iter_cond_refs(c.expr)]
            for c in self._doc.conditions
        }
        line_map: Dict[str, int] = {c.name: c.line for c in self._doc.conditions}

        WHITE, GRAY, BLACK = 0, 1, 2
        color: Dict[str, int] = {n: WHITE for n in graph}
        reported: Set[FrozenSet[str]] = set()

        def dfs(node: str, path: List[str]) -> None:
            if node not in graph:
                return  # ref a nome esterno, catturata da V-001
            if color[node] == GRAY:
                # Ciclo trovato
                idx = path.index(node)
                cycle = path[idx:]
                key: FrozenSet[str] = frozenset(cycle)
                if key not in reported:
                    reported.add(key)
                    cycle_str = " -> ".join(cycle) + f" -> {node}"
                    self._err(line_map.get(node, 0), "V-008",
                              f"ciclo in @conditions: {cycle_str}")
                return
            if color[node] == BLACK:
                return
            color[node] = GRAY
            path.append(node)
            for dep in graph.get(node, []):
                dfs(dep, path)
            path.pop()
            color[node] = BLACK

        for name in graph:
            if color[name] == WHITE:
                dfs(name, [])

    # -----------------------------------------------------------------------
    # V-009 — [stop] after [continue/optional] in same chain — WARNING
    # -----------------------------------------------------------------------

    def _check_v009_stop_after_continue(self) -> None:
        all_chains: List[Tuple[str, List[EffectEntry]]] = []
        for rule in self._doc.rules:
            all_chains.append((f"regola '{rule.name}'", rule.effects))
        for trig in self._doc.triggers:
            all_chains.append((f"trigger '{trig.name}'", trig.effects))
        for st in self._doc.statuses:
            for hook in st.hooks:
                all_chains.append(
                    (f"status '{st.name}' {hook.hook_type}", hook.chain)
                )

        for context, chain in all_chains:
            for i in range(len(chain) - 1):
                curr = self._resolve_stop_on_failure(chain[i])
                nxt = self._resolve_stop_on_failure(chain[i + 1])
                if curr is False and nxt is True:
                    self._warn(chain[i + 1].line, "V-009",
                               f"{context}: effetto con [stop] segue un effetto"
                               f" con [continue/optional] — comportamento insolito")

    def _resolve_stop_on_failure(self, entry: EffectEntry) -> Optional[bool]:
        """Risolve stop_on_failure per un EffectEntry (inline o named)."""
        if entry.call is not None:
            return entry.call.stop_on_failure
        if entry.optional:
            return False   # suffisso ? override
        if entry.name and entry.name in self._effect_map:
            return self._effect_map[entry.name].call.stop_on_failure
        return None

    # -----------------------------------------------------------------------
    # V-010 — APPLY_STATUS with undefined status
    # -----------------------------------------------------------------------

    def _check_v010_apply_status(self) -> None:
        for call in self._iter_all_effect_calls():
            if call.effect_type == "APPLY_STATUS" and call.args:
                status_id = call.args[0]
                if status_id not in self._status_names:
                    self._err(call.line, "V-010",
                              f"APPLY_STATUS: status '{status_id}'"
                              f" non dichiarato in @statuses")

    # -----------------------------------------------------------------------
    # Traversal helpers
    # -----------------------------------------------------------------------

    def _iter_all_effect_calls(self) -> Iterator[EffectCall]:
        """Itera su tutti i nodi EffectCall nel documento."""
        for eff in self._doc.effects:
            yield eff.call
        for st in self._doc.statuses:
            for hook in st.hooks:
                yield from _iter_inline_calls(hook.chain)
        for rule in self._doc.rules:
            yield from _iter_inline_calls(rule.effects)
        for trig in self._doc.triggers:
            yield from _iter_inline_calls(trig.effects)

    def _iter_all_cond_atoms(self) -> Iterator[CondAtom]:
        """Itera su tutti i nodi CondAtom nel documento."""
        for cdef in self._doc.conditions:
            yield from _iter_cond_atoms(cdef.expr)
        for rule in self._doc.rules:
            if rule.condition:
                yield from _iter_cond_atoms(rule.condition)
        for trig in self._doc.triggers:
            if trig.condition:
                yield from _iter_cond_atoms(trig.condition)

    # -----------------------------------------------------------------------
    # Diagnostic helpers
    # -----------------------------------------------------------------------

    def _err(self, line: int, code: str, message: str) -> None:
        self._diags.append(Diagnostic("ERROR", line, code, message))

    def _warn(self, line: int, code: str, message: str) -> None:
        self._diags.append(Diagnostic("WARNING", line, code, message))


# ---------------------------------------------------------------------------
# Pure helpers (stateless)
# ---------------------------------------------------------------------------

def _iter_cond_atoms(expr: CondExpr) -> Iterator[CondAtom]:
    if isinstance(expr, CondAtom):
        yield expr
    elif isinstance(expr, CondNot):
        yield from _iter_cond_atoms(expr.operand)
    elif isinstance(expr, (CondAnd, CondOr)):
        yield from _iter_cond_atoms(expr.left)
        yield from _iter_cond_atoms(expr.right)


def _iter_cond_refs(expr: CondExpr) -> Iterator[CondRef]:
    if isinstance(expr, CondRef):
        yield expr
    elif isinstance(expr, CondNot):
        yield from _iter_cond_refs(expr.operand)
    elif isinstance(expr, (CondAnd, CondOr)):
        yield from _iter_cond_refs(expr.left)
        yield from _iter_cond_refs(expr.right)


def _iter_inline_calls(chain: List[EffectEntry]) -> Iterator[EffectCall]:
    for entry in chain:
        if entry.call is not None:
            yield entry.call


def _iter_strings_in_cond(expr: CondExpr) -> Iterator[Tuple[str, int]]:
    """Yields (arg_value, line) per tutti gli argomenti nelle CondAtom del tree."""
    if isinstance(expr, CondAtom):
        for a in expr.args:
            yield a, expr.line
    elif isinstance(expr, CondNot):
        yield from _iter_strings_in_cond(expr.operand)
    elif isinstance(expr, (CondAnd, CondOr)):
        yield from _iter_strings_in_cond(expr.left)
        yield from _iter_strings_in_cond(expr.right)


def _call_all_args(call: EffectCall) -> List[str]:
    """Restituisce tutti i valori stringa dell'EffectCall (target_ref + args)."""
    result = list(call.args)
    if call.target_ref:
        result.append(call.target_ref)
    return result
