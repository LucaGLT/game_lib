"""
yaml_gen.py — Genera YAML canonico da AST GRS v0.3 (Fase 5).

Implementazione stdlib-only: nessuna dipendenza esterna.
Il mapping segue la tabella "GRS → YAML" della spec grs-spec.md.

Struttura output:
    meta:
    targets:
    conditions:
    effects:
    statuses:
    rules:
    triggers:
"""

from __future__ import annotations
import re
from typing import Any, Dict, List, Optional, Union

from .ast_nodes import (
    GrsDocument, MetaBlock, TargetDef, ConditionDef, EffectDef,
    StatusDef, RuleDef, TriggerDef,
    CondExpr, CondAtom, CondRef, CondNot, CondAnd, CondOr,
    EffectCall, EffectEntry,
)

YamlVal = Union[Dict, List, str, int, bool, None]

# ---------------------------------------------------------------------------
# Tabelle di mapping argomenti posizionali
# ---------------------------------------------------------------------------

# effect_type → nomi dei campi per call.args (escluso target_ref)
_EFFECT_FIELDS: Dict[str, List[str]] = {
    "MOVE_ACTOR":        ["value_ref"],
    "DEAL_DAMAGE":       ["amount"],
    "HEAL":              ["amount"],
    "APPLY_STATUS":      ["status_id"],
    "REMOVE_STATUS":     ["status_id"],
    "ADD_TAG":           ["tag"],
    "REMOVE_TAG":        ["tag"],
    "DRAW_CARDS":        ["deck_ref", "amount"],
    "MOVE_CARD_TO_ZONE": ["zone"],
    "EMIT_EVENT":        ["event_name"],
    "MANUAL_EFFECT":     ["event_name"],
}

# cond_type → nomi dei campi per atom.args
_COND_FIELDS: Dict[str, List[str]] = {
    "ACTOR_EXISTS":         ["subject_id_ref"],
    "ACTOR_HAS_TAG":        ["subject_id_ref", "tag"],
    "ACTOR_HAS_STATUS":     ["subject_id_ref", "status_id"],
    "ACTOR_HP_AT_OR_BELOW": ["subject_id_ref", "amount"],
    "ACTOR_HP_AT_OR_ABOVE": ["subject_id_ref", "amount"],
    "ACTOR_IN_LOCATION":    ["subject_id_ref", "location_ref"],
    "LOCATION_EXISTS":      ["location_ref"],
    "LOCATION_HAS_TAG":     ["location_ref", "tag"],
    "LOCATION_IS_ADJACENT": ["location_a_ref", "location_b_ref"],
    "TARGET_EXISTS":        [],
    "TARGET_HAS_TAG":       ["tag"],
    "TARGET_HAS_STATUS":    ["status_id"],
    "DECK_HAS_AT_LEAST":    ["deck_ref", "amount"],
    "CARD_IN_ZONE":         ["card_ref", "zone"],
    "ALWAYS":               [],
    "NEVER":                [],
    "RESOURCE_AT_LEAST":    ["subject_id_ref", "amount"],
}

# hook_type GRS → chiave YAML
_HOOK_KEY: Dict[str, str] = {
    "ON_APPLY":      "on_apply",
    "ON_REMOVE":     "on_remove",
    "ON_TURN_START": "on_turn_start",
    "ON_TURN_END":   "on_turn_end",
}


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def generate(doc: GrsDocument) -> str:
    """Genera la stringa YAML canonica dal documento GRS."""
    sections: List[str] = []

    if doc.meta:
        sections.append(_section("meta", _meta_to_dict(doc.meta)))

    if doc.targets:
        sections.append(_section_list("targets",
                                      [_target_to_dict(t) for t in doc.targets]))

    if doc.conditions:
        sections.append(_section_list("conditions",
                                      [_condition_def_to_dict(c) for c in doc.conditions]))

    if doc.effects:
        sections.append(_section_list("effects",
                                      [_effect_def_to_dict(e) for e in doc.effects]))

    if doc.statuses:
        sections.append(_section_list("statuses",
                                      [_status_to_dict(s) for s in doc.statuses]))

    if doc.rules:
        sections.append(_section_list("rules",
                                      [_rule_to_dict(r) for r in doc.rules]))

    if doc.triggers:
        sections.append(_section_list("triggers",
                                      [_trigger_to_dict(t) for t in doc.triggers]))

    return "\n\n".join(sections) + "\n"


# ---------------------------------------------------------------------------
# Conversione nodi → dict
# ---------------------------------------------------------------------------

def _meta_to_dict(m: MetaBlock) -> Dict:
    d: Dict = {
        "game_id":        m.game_id,
        "namespace":      m.namespace,
        "schema_version": m.version,
    }
    if m.min_gmrules:
        d["compatibility"] = {"gmRules_min": m.min_gmrules}
    return d


def _target_to_dict(t: TargetDef) -> Dict:
    d: Dict = {
        "id":       t.name,
        "kind":     t.kind,
        "selector": t.selector,
    }
    if t.range_type:
        d["range_type"] = t.range_type
        if t.range_n is not None:
            d["range_n"] = t.range_n
    else:
        d["range_type"] = "NONE"
    d["required"]   = t.required
    d["allow_self"] = t.allow_self
    if t.needs_tags:
        d["required_tags"] = t.needs_tags
    if t.forbids_tags:
        d["forbidden_tags"] = t.forbids_tags
    return d


def _condition_def_to_dict(c: ConditionDef) -> Dict:
    d: Dict = {"id": c.name}
    d.update(_cond_expr_to_dict(c.expr))
    return d


def _cond_expr_to_dict(expr: CondExpr) -> Dict:
    if isinstance(expr, CondRef):
        return {"ref": expr.name}

    if isinstance(expr, CondAtom):
        d: Dict = {"type": expr.cond_type}
        fields = _COND_FIELDS.get(expr.cond_type, [])
        for i, fname in enumerate(fields):
            if i < len(expr.args):
                d[fname] = _try_int(expr.args[i])
        return d

    if isinstance(expr, CondAnd):
        return {
            "op": "ALL_OF",
            "children": _cond_children(expr),
        }

    if isinstance(expr, CondOr):
        return {
            "op": "ANY_OF",
            "children": _cond_children(expr),
        }

    if isinstance(expr, CondNot):
        return {
            "op": "NOT",
            "children": [_cond_expr_to_dict(expr.operand)],
        }

    return {}


def _cond_children(expr: Union[CondAnd, CondOr]) -> List[Dict]:
    """Appiattisce AND/OR annidati in una lista di figli."""
    op_type = type(expr)
    result: List[Dict] = []

    def _collect(e: CondExpr) -> None:
        if type(e) == op_type:
            _collect(e.left)   # type: ignore[union-attr]
            _collect(e.right)  # type: ignore[union-attr]
        else:
            result.append(_cond_expr_to_dict(e))

    _collect(expr)
    return result


def _effect_def_to_dict(e: EffectDef) -> Dict:
    d: Dict = {"id": e.name}
    d.update(_effect_call_to_dict(e.call))
    return d


def _effect_call_to_dict(call: EffectCall) -> Dict:
    d: Dict = {"type": call.effect_type}
    if call.target_ref:
        d["target"] = call.target_ref
    fields = _EFFECT_FIELDS.get(call.effect_type, [])
    for i, fname in enumerate(fields):
        if i < len(call.args):
            d[fname] = _try_int(call.args[i])
    d["optional"]        = call.optional
    d["stop_on_failure"] = call.stop_on_failure
    return d


def _effect_entry_to_dict(entry: EffectEntry) -> Dict:
    if entry.call is not None:
        return _effect_call_to_dict(entry.call)
    d: Dict = {"ref": entry.name}
    if entry.optional:
        d["optional"] = True
    return d


def _status_to_dict(s: StatusDef) -> Dict:
    d: Dict = {
        "id": s.name,
        "stacking_policy": {"mode": s.stacking_mode},
        "default_duration": {"type": s.duration_type},
    }
    if s.amount is not None:
        d["default_duration"]["amount"] = s.amount
    if s.value is not None:
        d["default_duration"]["value"] = s.value

    for hook in s.hooks:
        key = _HOOK_KEY.get(hook.hook_type, hook.hook_type.lower())
        d[key] = [_effect_entry_to_dict(e) for e in hook.chain]

    return d


def _rule_to_dict(r: RuleDef) -> Dict:
    d: Dict = {
        "id":       r.name,
        "priority": r.priority,
        "enabled":  r.enabled,
        "target":   r.target,
    }
    if r.condition is not None:
        d["conditions"] = _cond_expr_to_dict(r.condition)
    d["effects"] = [_effect_entry_to_dict(e) for e in r.effects]
    return d


def _trigger_to_dict(t: TriggerDef) -> Dict:
    d: Dict = {
        "id":       t.name,
        "priority": t.priority,
        "enabled":  t.enabled,
        "type":     "ON_" + t.event_type,   # spec: ON_EVENT ACTOR_MOVED → trigger.type: ON_ACTOR_MOVED
    }
    if t.condition is not None:
        d["conditions"] = _cond_expr_to_dict(t.condition)
    d["effects"] = [_effect_entry_to_dict(e) for e in t.effects]
    return d


# ---------------------------------------------------------------------------
# Serializzatore YAML minimalista (stdlib only)
# ---------------------------------------------------------------------------

def _section(key: str, value: Dict) -> str:
    return f"{key}:\n{_to_yaml(value, 1)}"


def _section_list(key: str, items: List[Dict]) -> str:
    return f"{key}:\n{_to_yaml(items, 1)}"


def _to_yaml(obj: YamlVal, indent: int) -> str:
    pad = "  " * indent

    if isinstance(obj, dict):
        if not obj:
            return f"{pad}{{}}"
        lines = []
        for k, v in obj.items():
            if isinstance(v, dict) and v:
                lines.append(f"{pad}{k}:")
                lines.append(_to_yaml(v, indent + 1))
            elif isinstance(v, list) and v:
                lines.append(f"{pad}{k}:")
                lines.append(_to_yaml(v, indent + 1))
            else:
                lines.append(f"{pad}{k}: {_scalar(v)}")
        return "\n".join(lines)

    if isinstance(obj, list):
        if not obj:
            return f"{pad}[]"
        parts = []
        for item in obj:
            if isinstance(item, dict):
                parts.append(_list_item_dict(item, indent))
            else:
                parts.append(f"{pad}- {_scalar(item)}")
        return "\n".join(parts)

    return f"{pad}{_scalar(obj)}"


def _list_item_dict(item: Dict, indent: int) -> str:
    """Serializza un dict come elemento di lista YAML (prefisso `- `)."""
    pad = "  " * indent
    lines = []
    first = True
    for k, v in item.items():
        if first:
            prefix = f"{pad}- "
            extra  = "  "        # allineamento con il contenuto dopo "- "
            first = False
        else:
            prefix = f"{pad}  "
            extra  = ""

        if isinstance(v, dict) and v:
            lines.append(f"{prefix}{k}:")
            lines.append(_to_yaml(v, indent + 2))
        elif isinstance(v, list) and v:
            lines.append(f"{prefix}{k}:")
            lines.append(_to_yaml(v, indent + 2))
        else:
            lines.append(f"{prefix}{k}: {_scalar(v)}")

    return "\n".join(lines)


def _try_int(s: str) -> Union[int, str]:
    """Converte una stringa in int se è un numero intero puro, altrimenti la lascia."""
    if re.match(r'^-?\d+$', s):
        return int(s)
    return s


def _scalar(v: Any) -> str:
    if v is None:
        return "null"
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, int):
        return str(v)
    if isinstance(v, str):
        return _quote(v)
    return str(v)


def _quote(s: str) -> str:
    """Quota la stringa se contiene caratteri speciali YAML."""
    if not s:
        return '""'
    # Parole riservate YAML
    if s.lower() in ("true", "false", "null", "yes", "no", "on", "off"):
        return f'"{s}"'
    # Caratteri speciali
    if re.search(r'[:{}\[\]#&*?,|<>=!%@`\n\r\t]', s):
        return f'"{s}"'
    # Inizia con spazio o carattere speciale
    if s[0] in " \t-+.":
        return f'"{s}"'
    # Sembra un numero
    try:
        float(s)
        return f'"{s}"'
    except ValueError:
        pass
    return s
