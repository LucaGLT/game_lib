"""
ast_nodes.py — Nodi AST per il parser GRS.

Ogni dataclass rappresenta un costrutto del linguaggio GRS v0.3.
Ogni nodo porta il campo `line` con il numero di riga sorgente (1-based)
per consentire messaggi di errore precisi in lint e validate.
"""

from __future__ import annotations
from dataclasses import dataclass, field
from typing import List, Optional, Union


# ---------------------------------------------------------------------------
# @meta
# ---------------------------------------------------------------------------

@dataclass
class MetaBlock:
    game_id: str
    namespace: str
    version: str
    min_gmrules: Optional[str]
    line: int


# ---------------------------------------------------------------------------
# @targets
# ---------------------------------------------------------------------------

@dataclass
class TargetDef:
    name: str
    kind: str                       # ACTOR, LOCATION, CARD, DECK, ITEM, ...
    selector: str                   # SELF, SELECTED_ACTOR, ALL_ENEMIES_IN_LOCATION, ...
    range_type: Optional[str]       # SAME_LOCATION, ADJACENT_LOCATION, WITHIN_N_LOCATIONS, ...
    range_n: Optional[int]          # N per WITHIN_N_LOCATIONS
    required: bool                  # True = required, False = optional
    allow_self: bool                # False quando no_self è presente
    needs_tags: List[str]           # tags richiesti
    forbids_tags: List[str]         # tags vietati
    line: int


# ---------------------------------------------------------------------------
# @conditions — espressioni composte
# ---------------------------------------------------------------------------

# Condizione atomica: tipo + lista argomenti
@dataclass
class CondAtom:
    cond_type: str                  # ACTOR_EXISTS, ACTOR_HAS_TAG, ...
    args: List[str]                 # argomenti posizionali (ref runtime o literal)
    line: int


# Condizione riferita per nome (es. C_HeroAlive)
@dataclass
class CondRef:
    name: str
    line: int


# Operatori booleani
@dataclass
class CondNot:
    operand: "CondExpr"
    line: int


@dataclass
class CondAnd:
    left: "CondExpr"
    right: "CondExpr"
    line: int


@dataclass
class CondOr:
    left: "CondExpr"
    right: "CondExpr"
    line: int


# Tipo unione per qualsiasi espressione condizionale
CondExpr = Union[CondAtom, CondRef, CondNot, CondAnd, CondOr]


@dataclass
class ConditionDef:
    name: str
    expr: CondExpr
    line: int


# ---------------------------------------------------------------------------
# @effects
# ---------------------------------------------------------------------------

@dataclass
class EffectCall:
    """Chiamata inline di effetto (usata in @statuses, @triggers)."""
    effect_type: str                # MOVE_ACTOR, DEAL_DAMAGE, APPLY_STATUS, ...
    target_ref: Optional[str]       # nome target o ref runtime; None per MANUAL_EFFECT
    args: List[str]                 # argomenti aggiuntivi (amount, status_id, ...)
    optional: bool                  # [optional]
    stop_on_failure: bool           # [stop] — default True
    line: int


@dataclass
class EffectEntry:
    """Elemento di una effect chain: nome definito oppure chiamata inline."""
    name: Optional[str]             # riferimento a EffectDef (es. E_MoveHero)
    call: Optional[EffectCall]      # chiamata inline (solo in @statuses / @triggers)
    optional: bool                  # suffisso ? sulla riga
    line: int


@dataclass
class EffectDef:
    name: str
    call: EffectCall
    line: int


# ---------------------------------------------------------------------------
# @statuses
# ---------------------------------------------------------------------------

@dataclass
class StatusHook:
    hook_type: str                  # ON_APPLY, ON_REMOVE, ON_TURN_START, ON_TURN_END
    chain: List[EffectEntry]
    line: int


@dataclass
class StatusDef:
    name: str
    stacking_mode: str              # ONE_ONLY, REFRESH, ADD_STACK, REPLACE, UNIQUE_BY_SOURCE
    duration_type: str              # PERMANENT, UNTIL_REMOVED, FOR_N, UNTIL_NEXT_TURN, WHILE_IN_LOCATION
    amount: Optional[int]           # per FOR_N
    value: Optional[str]            # per WHILE_IN_LOCATION
    hooks: List[StatusHook]
    line: int


# ---------------------------------------------------------------------------
# @rules
# ---------------------------------------------------------------------------

@dataclass
class RuleDef:
    name: str
    priority: int                   # default 0 se assente
    enabled: bool                   # False se [disabled]
    condition: Optional[CondExpr]   # None se assente (IF omessa)
    target: str                     # nome target
    effects: List[EffectEntry]      # effect chain
    line: int


# ---------------------------------------------------------------------------
# @triggers
# ---------------------------------------------------------------------------

@dataclass
class TriggerDef:
    name: str
    priority: int
    enabled: bool
    event_type: str                 # ACTION_SUBMITTED, ACTION_COMPLETED, ACTOR_MOVED, ...
    condition: Optional[CondExpr]
    effects: List[EffectEntry]
    line: int


# ---------------------------------------------------------------------------
# Documento completo
# ---------------------------------------------------------------------------

@dataclass
class GrsDocument:
    meta: Optional[MetaBlock]
    targets: List[TargetDef]        = field(default_factory=list)
    conditions: List[ConditionDef]  = field(default_factory=list)
    effects: List[EffectDef]        = field(default_factory=list)
    statuses: List[StatusDef]       = field(default_factory=list)
    rules: List[RuleDef]            = field(default_factory=list)
    triggers: List[TriggerDef]      = field(default_factory=list)
