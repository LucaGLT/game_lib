"""
graph_gen.py — Genera diagrammi Mermaid da AST GRS v0.3 (Fase 6).

Stile approvato:
  - Rettangolo  BLU    → @rules    (rettangolo con bordo blu)
  - Esagono     GIALLO → @conditions
  - Subgraph    GIALLO → gruppo AND / OR
  - Rettangolo  VERDE  → @effects  (effect chain)
  - Rettangolo  ARANCIO→ @statuses (target di APPLY_STATUS, freccia tratteggiata)
  - Cerchio     VIOLA  → @targets
  - Rettangolo  ROSSO  → @triggers (header trigger)
  - Rombo       ROSSO  → ON_EVENT  (evento che scatena il trigger)

Logica trigger connessi per --rule NAME:
  - ACTION_SUBMITTED : freccia tratteggiata dal nodo regola
  - ACTION_COMPLETED + controlla status applicato dalla regola
                     : freccia tratteggiata dall'ultimo effetto
  - ACTOR_MOVED      + regola usa MOVE_ACTOR
                     : freccia tratteggiata dall'ultimo effetto
  - Altri trigger che controllano status applicati dalla regola
                     : freccia tratteggiata dall'ultimo effetto
"""

from __future__ import annotations
import re
from typing import Dict, Iterator, List, Optional, Set, Tuple

from .ast_nodes import (
    GrsDocument, RuleDef, TriggerDef, ConditionDef, EffectDef,
    TargetDef, StatusDef, EffectCall, EffectEntry,
    CondExpr, CondAtom, CondRef, CondNot, CondAnd, CondOr,
)

# ---------------------------------------------------------------------------
# Colori Mermaid
# ---------------------------------------------------------------------------

_STYLE_RULE    = "fill:#CCE5FF,stroke:#3399FF,stroke-width:2px"
_STYLE_COND    = "fill:#FFF3CD,stroke:#FFAA00"
_STYLE_SUBGR   = "fill:#FFFDE7,stroke:#F9A825"
_STYLE_EFFECT  = "fill:#C8E6C9,stroke:#388E3C"
_STYLE_STATUS  = "fill:#FFE0B2,stroke:#E65100"
_STYLE_TARGET  = "fill:#E1BEE7,stroke:#7B1FA2"
_STYLE_TRIGGER = "fill:#FFCDD2,stroke:#C62828"


# ---------------------------------------------------------------------------
# Helpers di ID e nodi Mermaid
# ---------------------------------------------------------------------------

def _nid(*parts: str) -> str:
    """Genera un ID Mermaid sicuro (no punti, no trattini)."""
    joined = "_".join(str(p) for p in parts)
    return re.sub(r"[^A-Za-z0-9_]", "_", joined)


def _rect(nid: str, label: str) -> str:
    return f'    {nid}["{label}"]'


def _hexagon(nid: str, label: str) -> str:
    return f'    {nid}{{{{"{label}"}}}}'


def _diamond(nid: str, label: str) -> str:
    return f'    {nid}{{"{label}"}}'


def _circle(nid: str, label: str) -> str:
    return f'    {nid}(("{label}"))'


# ---------------------------------------------------------------------------
# GraphGen
# ---------------------------------------------------------------------------

class GraphGen:

    def __init__(self, doc: GrsDocument) -> None:
        self._doc      = doc
        self._cond_map: Dict[str, ConditionDef] = {c.name: c for c in doc.conditions}
        self._eff_map:  Dict[str, EffectDef]    = {e.name: e for e in doc.effects}
        self._tgt_map:  Dict[str, TargetDef]    = {t.name: t for t in doc.targets}
        self._st_map:   Dict[str, StatusDef]    = {s.name: s for s in doc.statuses}
        self._sg_count  = 0          # contatore subgraph per IDs unici

    # -----------------------------------------------------------------------
    # API pubblica
    # -----------------------------------------------------------------------

    def generate_rule(self, rule_name: str) -> str:
        """Genera Mermaid per una singola regola + trigger connessi."""
        rule = next((r for r in self._doc.rules if r.name == rule_name), None)
        if rule is None:
            return f"<!-- rule '{rule_name}' not found -->\n"
        self._sg_count = 0
        return self._build_mermaid(rule, self._find_connected_triggers(rule))

    def generate_all(self) -> str:
        """Genera un .md con una sezione Mermaid per ogni regola abilitata."""
        sections: List[str] = []
        for rule in self._doc.rules:
            if not rule.enabled:
                continue
            self._sg_count = 0
            graph = self._build_mermaid(rule, self._find_connected_triggers(rule))
            sections.append(f"## {rule.name}\n\n{graph}")
        return "\n\n---\n\n".join(sections)

    # -----------------------------------------------------------------------
    # Logica connessione trigger
    # -----------------------------------------------------------------------

    def _statuses_applied(self, rule: RuleDef) -> Set[str]:
        result: Set[str] = set()
        for entry in rule.effects:
            call = self._resolve_call(entry)
            if call and call.effect_type == "APPLY_STATUS" and call.args:
                result.add(call.args[0])
        return result

    def _effect_types(self, rule: RuleDef) -> Set[str]:
        result: Set[str] = set()
        for entry in rule.effects:
            call = self._resolve_call(entry)
            if call:
                result.add(call.effect_type)
        return result

    def _trigger_checks_status(self, trig: TriggerDef, statuses: Set[str]) -> bool:
        if not trig.condition or not statuses:
            return False
        for atom in _iter_atoms(trig.condition):
            if atom.cond_type == "ACTOR_HAS_STATUS" and len(atom.args) >= 2:
                if atom.args[1] in statuses:
                    return True
            if atom.cond_type == "TARGET_HAS_STATUS" and len(atom.args) >= 1:
                if atom.args[0] in statuses:
                    return True
        return False

    def _find_connected_triggers(self, rule: RuleDef) -> List[TriggerDef]:
        applied = self._statuses_applied(rule)
        eff_types = self._effect_types(rule)
        result: List[TriggerDef] = []
        seen: Set[str] = set()

        for trig in self._doc.triggers:
            if trig.name in seen:
                continue
            connected = False
            if trig.event_type == "ACTION_SUBMITTED":
                connected = True
            elif trig.event_type == "ACTION_COMPLETED" and self._trigger_checks_status(trig, applied):
                connected = True
            elif trig.event_type == "ACTOR_MOVED" and "MOVE_ACTOR" in eff_types:
                connected = True
            elif self._trigger_checks_status(trig, applied):
                connected = True

            if connected:
                result.append(trig)
                seen.add(trig.name)

        return result

    # -----------------------------------------------------------------------
    # Costruttore Mermaid principale
    # -----------------------------------------------------------------------

    def _build_mermaid(self, rule: RuleDef, triggers: List[TriggerDef]) -> str:
        lines:  List[str] = ["graph TD"]
        styles: List[str] = []

        # --- Regola ---
        rule_id = _nid("R", rule.name)
        lines.append(_rect(rule_id, f"{rule.name}\\npriority = {rule.priority}"))
        styles.append(f"    style {rule_id} {_STYLE_RULE}")

        # --- Condizione ---
        cond_exit = self._add_condition(rule.condition, rule.name, rule_id,
                                        lines, styles)
        prev = cond_exit if cond_exit else rule_id

        # --- Effect chain ---
        eff_ids: List[str] = []
        applied_st: List[Tuple[str, str]] = []   # (status_name, eff_id)

        for i, entry in enumerate(rule.effects):
            call  = self._resolve_call(entry)
            eid   = _nid("E", rule.name, str(i))
            label = self._effect_label(entry, call)
            lines.append(_rect(eid, label))
            lines.append(f"    {prev} --> {eid}")
            styles.append(f"    style {eid} {_STYLE_EFFECT}")
            eff_ids.append(eid)
            prev = eid

            if call and call.effect_type == "APPLY_STATUS" and call.args:
                applied_st.append((call.args[0], eid))

        last_eff = prev  # rule_id if no effects

        # --- Target ---
        if rule.target:
            tid    = _nid("TGT", rule.name)
            tdef   = self._tgt_map.get(rule.target)
            sel    = tdef.selector if tdef else "?"
            lines.append(_circle(tid, f"{rule.target}\\n{sel}"))
            lines.append(f"    {last_eff} --> {tid}")
            styles.append(f"    style {tid} {_STYLE_TARGET}")

        # --- Status nodes (dashed da APPLY_STATUS) ---
        for sname, eid in applied_st:
            sid   = _nid("ST", rule.name, sname)
            sdef  = self._st_map.get(sname)
            mode  = f"{sdef.stacking_mode} · {sdef.duration_type}" if sdef else "?"
            lines.append(_rect(sid, f"{sname}\\n{mode}"))
            lines.append(f"    {eid} -.-> {sid}")
            styles.append(f"    style {sid} {_STYLE_STATUS}")

        # --- Trigger connessi ---
        for trig in triggers:
            self._add_trigger(trig, rule_id, last_eff, lines, styles)

        lines.append("")
        lines.extend(styles)
        return "```mermaid\n" + "\n".join(lines) + "\n```\n"

    # -----------------------------------------------------------------------
    # Nodi condizione
    # -----------------------------------------------------------------------

    def _add_condition(
        self,
        cond: Optional[CondExpr],
        rule_name: str,
        from_id: str,
        lines: List[str],
        styles: List[str],
    ) -> Optional[str]:
        """
        Aggiunge i nodi condizione della regola.
        Ritorna l'ID del nodo di "uscita" (a cui si collega la chain effetti).
        """
        if cond is None:
            return None

        if isinstance(cond, CondRef):
            return self._add_cond_ref(cond.name, rule_name, from_id, lines, styles)

        if isinstance(cond, CondAtom):
            cid   = _nid("C", rule_name, cond.cond_type)
            label = self._atom_label(cond)
            lines.append(_hexagon(cid, label))
            lines.append(f"    {from_id} --> {cid}")
            styles.append(f"    style {cid} {_STYLE_COND}")
            return cid

        if isinstance(cond, (CondAnd, CondOr)):
            op = "AND" if isinstance(cond, CondAnd) else "OR"
            return self._add_composite_subgraph(cond, op, rule_name, from_id,
                                                None, lines, styles)

        if isinstance(cond, CondNot):
            return self._add_condition(cond.operand, rule_name, from_id,
                                       lines, styles)

        return None

    def _add_cond_ref(
        self,
        name: str,
        rule_name: str,
        from_id: str,
        lines: List[str],
        styles: List[str],
    ) -> str:
        """Mostra la condizione per nome; espande come subgraph se AND/OR."""
        cid = _nid("C", rule_name, name)
        lines.append(_hexagon(cid, name))
        lines.append(f"    {from_id} --> {cid}")
        styles.append(f"    style {cid} {_STYLE_COND}")

        cdef = self._cond_map.get(name)
        if cdef and isinstance(cdef.expr, (CondAnd, CondOr)):
            op = "AND" if isinstance(cdef.expr, CondAnd) else "OR"
            sg_id = self._add_composite_subgraph(
                cdef.expr, op, rule_name, cid, None, lines, styles
            )
            return sg_id

        return cid

    def _add_composite_subgraph(
        self,
        cond: Union[CondAnd, CondOr],
        op: str,
        rule_name: str,
        from_id: str,
        edge_label: Optional[str],
        lines: List[str],
        styles: List[str],
    ) -> str:
        self._sg_count += 1
        sg_id = _nid("AG", rule_name, op, str(self._sg_count))
        children = self._cond_children_nodes(cond, rule_name)

        lines.append(f'    subgraph {sg_id}[" {op} "]')
        lines.append(f"        direction LR")
        for ch_id, ch_label in children:
            lines.append(f"        {_hexagon(ch_id, ch_label).strip()}")
            styles.append(f"    style {ch_id} {_STYLE_COND}")
        lines.append("    end")

        lbl = f" -- {op} --> " if edge_label is None else f" -- {op} --> "
        lines.append(f"    {from_id}{lbl}{sg_id}")
        styles.append(f"    style {sg_id} {_STYLE_SUBGR}")
        return sg_id

    def _cond_children_nodes(
        self, expr: CondExpr, rule_name: str
    ) -> List[Tuple[str, str]]:
        """
        Raccoglie (node_id, label) per i figli diretti di AND/OR,
        appiattendo AND/OR annidati dello stesso tipo.
        """
        op_type = type(expr)
        result: List[Tuple[str, str]] = []

        def _collect(e: CondExpr) -> None:
            if type(e) == op_type:
                _collect(e.left)   # type: ignore[union-attr]
                _collect(e.right)  # type: ignore[union-attr]
            elif isinstance(e, CondRef):
                result.append((_nid("C", rule_name, e.name), e.name))
            elif isinstance(e, CondAtom):
                result.append((_nid("C", rule_name, e.cond_type),
                                self._atom_label(e)))
            else:
                nid = _nid("C", rule_name, "expr", str(len(result)))
                result.append((nid, self._inline_cond_label(e)))

        _collect(expr)
        return result

    # -----------------------------------------------------------------------
    # Nodi trigger
    # -----------------------------------------------------------------------

    def _add_trigger(
        self,
        trig: TriggerDef,
        rule_id: str,
        last_eff_id: str,
        lines: List[str],
        styles: List[str],
    ) -> None:
        tr_id = _nid("TR", trig.name)
        ev_id = _nid("EV", trig.name)

        lines.append(_rect(tr_id, f"{trig.name}\\npriority = {trig.priority}"))
        lines.append(_diamond(ev_id, f"ON_EVENT\\n{trig.event_type}"))
        lines.append(f"    {tr_id} --> {ev_id}")
        styles.append(f"    style {tr_id} {_STYLE_TRIGGER}")
        styles.append(f"    style {ev_id} {_STYLE_TRIGGER}")

        # Collegamento alla regola (tratteggiato)
        if trig.event_type == "ACTION_SUBMITTED":
            lines.append(f"    {rule_id} -.-> {ev_id}")
        else:
            lines.append(f"    {last_eff_id} -.-> {ev_id}")

        prev = ev_id

        # Condizione trigger
        if trig.condition:
            tc_id  = _nid("TC", trig.name)
            label  = self._inline_cond_label(trig.condition)
            lines.append(_hexagon(tc_id, label))
            lines.append(f"    {prev} --> {tc_id}")
            styles.append(f"    style {tc_id} {_STYLE_COND}")
            prev = tc_id

        # Effect chain trigger
        for i, entry in enumerate(trig.effects):
            call  = self._resolve_call(entry)
            te_id = _nid("TE", trig.name, str(i))
            lines.append(_rect(te_id, self._effect_label(entry, call)))
            lines.append(f"    {prev} --> {te_id}")
            styles.append(f"    style {te_id} {_STYLE_EFFECT}")
            prev = te_id

    # -----------------------------------------------------------------------
    # Label helpers
    # -----------------------------------------------------------------------

    def _atom_label(self, atom: CondAtom) -> str:
        """Label esagono condizione: tipo + args non-runtime su riga separata."""
        non_ref = [a for a in atom.args
                   if not a.startswith(("input.", "event.", "source.", "target."))]
        if non_ref:
            return f"{atom.cond_type}\\n{', '.join(non_ref)}"
        return atom.cond_type

    def _inline_cond_label(self, expr: CondExpr) -> str:
        """Label compatto per condizioni trigger (una sola riga)."""
        if isinstance(expr, CondRef):
            return expr.name
        if isinstance(expr, CondAtom):
            return self._atom_label(expr)
        if isinstance(expr, CondNot):
            return f"NOT {self._inline_cond_label(expr.operand)}"
        if isinstance(expr, CondAnd):
            return (f"{self._inline_cond_label(expr.left)}"
                    f" AND {self._inline_cond_label(expr.right)}")
        if isinstance(expr, CondOr):
            return (f"{self._inline_cond_label(expr.left)}"
                    f" OR {self._inline_cond_label(expr.right)}")
        return "?"

    def _effect_label(self, entry: EffectEntry, call: Optional[EffectCall]) -> str:
        """Label rettangolo effetto: nome + tipo + modificatore."""
        name  = entry.name or (call.effect_type if call else "?")
        etype = ""
        if call:
            etype = call.effect_type
        elif entry.name and entry.name in self._eff_map:
            etype = self._eff_map[entry.name].call.effect_type

        is_opt = entry.optional or (call is not None and call.optional)
        suffix = " · opt" if is_opt else ""

        if etype and etype != name:
            return f"{name}\\n{etype}{suffix}"
        return f"{name}{suffix}"

    # -----------------------------------------------------------------------
    # Resolve call da EffectEntry
    # -----------------------------------------------------------------------

    def _resolve_call(self, entry: EffectEntry) -> Optional[EffectCall]:
        if entry.call is not None:
            return entry.call
        if entry.name and entry.name in self._eff_map:
            return self._eff_map[entry.name].call
        return None


# ---------------------------------------------------------------------------
# Pure helpers
# ---------------------------------------------------------------------------

def _iter_atoms(expr: CondExpr) -> Iterator[CondAtom]:
    if isinstance(expr, CondAtom):
        yield expr
    elif isinstance(expr, CondNot):
        yield from _iter_atoms(expr.operand)
    elif isinstance(expr, (CondAnd, CondOr)):
        yield from _iter_atoms(expr.left)
        yield from _iter_atoms(expr.right)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def grapho_rule(doc: GrsDocument, rule_name: str) -> str:
    """Mermaid block per una singola regola, con header."""
    gen = GraphGen(doc)
    return f"# GRS Graph — {rule_name}\n\n{gen.generate_rule(rule_name)}"


def grapho_all(doc: GrsDocument) -> str:
    """Documento .md con una sezione per ogni regola."""
    gen = GraphGen(doc)
    meta_title = doc.meta.game_id if doc.meta else "GRS"
    header = f"# GRS Rule Graphs — {meta_title}\n\n"
    return header + gen.generate_all()
