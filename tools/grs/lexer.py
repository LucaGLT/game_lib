"""
lexer.py — Tokenizzatore per file GRS v0.3.

Produce una lista di Token da una stringa sorgente.
Non ha dipendenze esterne: usa solo `re` dalla stdlib.

Token prodotti (TokenKind):
    BLOCK_OPEN   @meta | @targets | @conditions | @effects
                 @statuses | @rules | @triggers
    BLOCK_END    @end
    DEFINE       ::
    KEYWORD      IF ON THEN AND OR NOT
    AND_THEN     AND THEN  (coppia speciale, ha precedenza su AND + THEN separati)
    EVENT_KW     ON_EVENT
    HOOK_KW      ON_APPLY ON_REMOVE ON_TURN_START ON_TURN_END
    EFFECT_TYPE  MOVE_ACTOR DEAL_DAMAGE HEAL APPLY_STATUS REMOVE_STATUS
                 ADD_TAG REMOVE_TAG DRAW_CARDS MOVE_CARD_TO_ZONE
                 EMIT_EVENT MANUAL_EFFECT
    COND_TYPE    ACTOR_EXISTS ACTOR_HAS_TAG ACTOR_HAS_STATUS
                 ACTOR_HP_AT_OR_BELOW ACTOR_HP_AT_OR_ABOVE ACTOR_IN_LOCATION
                 LOCATION_EXISTS LOCATION_HAS_TAG LOCATION_IS_ADJACENT
                 TARGET_EXISTS TARGET_HAS_TAG TARGET_HAS_STATUS
                 DECK_HAS_AT_LEAST CARD_IN_ZONE ALWAYS NEVER
                 RESOURCE_AT_LEAST
    TARGET_KIND  ACTOR ACTOR_GROUP LOCATION CARD DECK ITEM INTERACTABLE NONE
    TARGET_SEL   SELF SOURCE SELECTED_ACTOR SELECTED_ALLY SELECTED_ENEMY
                 ALL_ACTORS_IN_LOCATION ALL_ALLIES_IN_LOCATION
                 ALL_ENEMIES_IN_LOCATION ACTORS_WITH_STATUS SELECTED
                 SELECTED_CARD SELECTED_ITEM MANUAL
    RANGE_KW     range
    RANGE_TYPE   SAME_LOCATION ADJACENT_LOCATION WITHIN_N_LOCATIONS
                 ANY_VISIBLE_LOCATION GLOBAL
    STACKING     ONE_ONLY REFRESH ADD_STACK REPLACE UNIQUE_BY_SOURCE
    DURATION     PERMANENT UNTIL_REMOVED FOR_N UNTIL_NEXT_TURN
                 WHILE_IN_LOCATION
    ATTR_KW      amount value required optional no_self needs forbids
                 disabled game ns version min_gmrules priority
    TRIGGER_TYPE ACTION_SUBMITTED ACTION_COMPLETED CARD_PLAYED ACTOR_DAMAGED
                 ACTOR_MOVED STATUS_APPLIED TIME_REACHED LOCATION_ENTERED
    MODIFIER     [optional] [stop] [continue]
    LPAREN       (
    RPAREN       )
    COMMA        ,
    QUESTION     ?
    BANG         !
    BRACKET_OPEN [
    BRACKET_CLOSE ]
    EQUALS       =
    INTEGER      sequenza di cifre (con eventuale segno -)
    REF          input.xxx | event.xxx | source.xxx | target.xxx
    IDENT        qualsiasi identificatore non classificato sopra
    COMMENT      # ...  (incluso nel token per debug, scartato dal parser)
    NEWLINE      \n   (solo per tracciare i numeri di riga)
"""

from __future__ import annotations
import re
from dataclasses import dataclass
from enum import Enum, auto
from typing import List


# ---------------------------------------------------------------------------
# TokenKind
# ---------------------------------------------------------------------------

class TokenKind(Enum):
    BLOCK_OPEN    = auto()
    BLOCK_END     = auto()
    DEFINE        = auto()
    AND_THEN      = auto()
    KEYWORD       = auto()
    EVENT_KW      = auto()
    HOOK_KW       = auto()
    EFFECT_TYPE   = auto()
    COND_TYPE     = auto()
    TARGET_KIND   = auto()
    TARGET_SEL    = auto()
    RANGE_KW      = auto()
    RANGE_TYPE    = auto()
    STACKING      = auto()
    DURATION      = auto()
    ATTR_KW       = auto()
    TRIGGER_TYPE  = auto()
    MODIFIER      = auto()
    LPAREN        = auto()
    RPAREN        = auto()
    COMMA         = auto()
    QUESTION      = auto()
    BANG          = auto()
    BRACKET_OPEN  = auto()
    BRACKET_CLOSE = auto()
    EQUALS        = auto()
    INTEGER       = auto()
    REF           = auto()
    IDENT         = auto()
    COMMENT       = auto()
    NEWLINE       = auto()
    EOF           = auto()


# ---------------------------------------------------------------------------
# Token
# ---------------------------------------------------------------------------

@dataclass
class Token:
    kind: TokenKind
    value: str
    line: int

    def __repr__(self) -> str:
        return f"Token({self.kind.name}, {self.value!r}, line={self.line})"


# ---------------------------------------------------------------------------
# Vocabolari
# ---------------------------------------------------------------------------

_BLOCK_OPENS: set = {
    "@meta", "@targets", "@conditions", "@effects",
    "@statuses", "@rules", "@triggers",
}

_EFFECT_TYPES: set = {
    "MOVE_ACTOR", "DEAL_DAMAGE", "HEAL", "APPLY_STATUS", "REMOVE_STATUS",
    "ADD_TAG", "REMOVE_TAG", "DRAW_CARDS", "MOVE_CARD_TO_ZONE",
    "EMIT_EVENT", "MANUAL_EFFECT",
}

_COND_TYPES: set = {
    "ACTOR_EXISTS", "ACTOR_HAS_TAG", "ACTOR_HAS_STATUS",
    "ACTOR_HP_AT_OR_BELOW", "ACTOR_HP_AT_OR_ABOVE", "ACTOR_IN_LOCATION",
    "LOCATION_EXISTS", "LOCATION_HAS_TAG", "LOCATION_IS_ADJACENT",
    "TARGET_EXISTS", "TARGET_HAS_TAG", "TARGET_HAS_STATUS",
    "DECK_HAS_AT_LEAST", "CARD_IN_ZONE", "ALWAYS", "NEVER",
    "RESOURCE_AT_LEAST",
}

_TARGET_KINDS: set = {
    "ACTOR", "ACTOR_GROUP", "CARD", "DECK", "ITEM",
    "INTERACTABLE", "NONE",
}

# LOCATION appare sia come TargetKind che come TargetSelector;
# il parser disambigua in base alla posizione.
_TARGET_SELS: set = {
    "SELF", "SOURCE", "SELECTED_ACTOR", "SELECTED_ALLY", "SELECTED_ENEMY",
    "ALL_ACTORS_IN_LOCATION", "ALL_ALLIES_IN_LOCATION",
    "ALL_ENEMIES_IN_LOCATION", "ACTORS_WITH_STATUS",
    "SELECTED", "SELECTED_CARD", "SELECTED_ITEM", "MANUAL", "LOCATION",
}

_RANGE_TYPES: set = {
    "SAME_LOCATION", "ADJACENT_LOCATION", "WITHIN_N_LOCATIONS",
    "ANY_VISIBLE_LOCATION", "GLOBAL",
}

_STACKING: set = {
    "ONE_ONLY", "REFRESH", "ADD_STACK", "REPLACE", "UNIQUE_BY_SOURCE",
}

_DURATION: set = {
    "PERMANENT", "UNTIL_REMOVED", "FOR_N", "UNTIL_NEXT_TURN",
    "WHILE_IN_LOCATION",
}

_TRIGGER_TYPES: set = {
    "ACTION_SUBMITTED", "ACTION_COMPLETED", "CARD_PLAYED",
    "ACTOR_DAMAGED", "ACTOR_MOVED", "STATUS_APPLIED",
    "TIME_REACHED", "LOCATION_ENTERED",
}

_KEYWORDS: set = {"IF", "ON", "THEN", "AND", "OR", "NOT"}

_HOOK_KW: set = {
    "ON_APPLY", "ON_REMOVE", "ON_TURN_START", "ON_TURN_END",
}

_ATTR_KW: set = {
    "amount", "value", "required", "optional", "no_self",
    "needs", "forbids", "disabled",
    "game", "ns", "version", "min_gmrules",
    "priority", "range",
}


def _classify_upper(word: str) -> TokenKind:
    """Classifica una parola UPPERCASE nel suo TokenKind."""
    if word in _EFFECT_TYPES:
        return TokenKind.EFFECT_TYPE
    if word in _COND_TYPES:
        return TokenKind.COND_TYPE
    if word in _TARGET_KINDS:
        return TokenKind.TARGET_KIND
    if word in _TARGET_SELS:
        return TokenKind.TARGET_SEL
    if word in _RANGE_TYPES:
        return TokenKind.RANGE_TYPE
    if word in _STACKING:
        return TokenKind.STACKING
    if word in _DURATION:
        return TokenKind.DURATION
    if word in _TRIGGER_TYPES:
        return TokenKind.TRIGGER_TYPE
    if word in _HOOK_KW:
        return TokenKind.HOOK_KW
    if word == "ON_EVENT":
        return TokenKind.EVENT_KW
    if word in _KEYWORDS:
        return TokenKind.KEYWORD
    return TokenKind.IDENT


# ---------------------------------------------------------------------------
# Regole regex (ordine importante: prima le più specifiche)
# ---------------------------------------------------------------------------

_TOKEN_RULES: List[tuple] = [
    # Commento: tutto dopo # fino a fine riga
    ("COMMENT",       r"#[^\n]*"),
    # Newline
    ("NEWLINE",       r"\n"),
    # Whitespace (scartato, non produce token)
    ("WS",            r"[ \t\r]+"),
    # Modifier brackets: [optional] [stop] [continue]
    ("MODIFIER",      r"\[(optional|stop|continue)\]"),
    # DEFINE ::
    ("DEFINE",        r"::"),
    # Block open / end
    ("BLOCK_END",     r"@end\b"),
    ("BLOCK_OPEN",    r"@(?:meta|targets|conditions|effects|statuses|rules|triggers)\b"),
    # Runtime ref: input.xxx, event.xxx, source.xxx, target.xxx
    ("REF",           r"(?:input|event|source|target)\.[A-Za-z_][A-Za-z0-9_.]*"),
    # Version string semver (es. 1.0.0, 0.5.0) — prima di INTEGER
    ("VERSION",        r"\d+\.\d+\.\d+(?:\.[A-Za-z0-9_]+)*"),
    # Integer (con segno opzionale)
    ("INTEGER",       r"-?\d+"),
    # Dotted identifier (es. dungeon.turn.started) — prima del semplice IDENT
    ("IDENT",         r"[A-Za-z_][A-Za-z0-9_.]*"),
    # Simboli singoli
    ("LPAREN",        r"\("),
    ("RPAREN",        r"\)"),
    ("COMMA",         r","),
    ("QUESTION",      r"\?"),
    ("BANG",          r"!"),
    ("BRACKET_OPEN",  r"\["),
    ("BRACKET_CLOSE", r"\]"),
    ("EQUALS",        r"="),
]

_MASTER_RE = re.compile(
    "|".join(f"(?P<{name}>{pattern})" for name, pattern in _TOKEN_RULES)
)


# ---------------------------------------------------------------------------
# LexerError
# ---------------------------------------------------------------------------

class LexerError(Exception):
    def __init__(self, message: str, line: int) -> None:
        super().__init__(f"[LEXER] line {line}: {message}")
        self.line = line


# ---------------------------------------------------------------------------
# tokenize
# ---------------------------------------------------------------------------

def tokenize(source: str) -> List[Token]:
    """
    Tokenizza la stringa `source` e restituisce la lista di Token.

    I token COMMENT e NEWLINE sono inclusi ma il parser può ignorarli.
    AND THEN viene riconosciuto come coppia speciale post-processing.
    """
    tokens: List[Token] = []
    current_line = 1

    for match in _MASTER_RE.finditer(source):
        kind_name = match.lastgroup
        value = match.group()

        if kind_name == "WS":
            continue
        if kind_name == "NEWLINE":
            tokens.append(Token(TokenKind.NEWLINE, value, current_line))
            current_line += 1
            continue
        if kind_name == "COMMENT":
            tokens.append(Token(TokenKind.COMMENT, value, current_line))
            continue
        if kind_name == "BLOCK_END":
            tokens.append(Token(TokenKind.BLOCK_END, value, current_line))
            continue
        if kind_name == "BLOCK_OPEN":
            tokens.append(Token(TokenKind.BLOCK_OPEN, value, current_line))
            continue
        if kind_name == "DEFINE":
            tokens.append(Token(TokenKind.DEFINE, value, current_line))
            continue
        if kind_name == "MODIFIER":
            tokens.append(Token(TokenKind.MODIFIER, value, current_line))
            continue
        if kind_name == "REF":
            tokens.append(Token(TokenKind.REF, value, current_line))
            continue
        if kind_name == "VERSION":
            # Versioni semver (1.0.0) trattate come IDENT opaco
            tokens.append(Token(TokenKind.IDENT, value, current_line))
            continue
        if kind_name == "INTEGER":
            tokens.append(Token(TokenKind.INTEGER, value, current_line))
            continue
        if kind_name == "LPAREN":
            tokens.append(Token(TokenKind.LPAREN, value, current_line))
            continue
        if kind_name == "RPAREN":
            tokens.append(Token(TokenKind.RPAREN, value, current_line))
            continue
        if kind_name == "COMMA":
            tokens.append(Token(TokenKind.COMMA, value, current_line))
            continue
        if kind_name == "QUESTION":
            tokens.append(Token(TokenKind.QUESTION, value, current_line))
            continue
        if kind_name == "BANG":
            tokens.append(Token(TokenKind.BANG, value, current_line))
            continue
        if kind_name == "BRACKET_OPEN":
            tokens.append(Token(TokenKind.BRACKET_OPEN, value, current_line))
            continue
        if kind_name == "BRACKET_CLOSE":
            tokens.append(Token(TokenKind.BRACKET_CLOSE, value, current_line))
            continue
        if kind_name == "EQUALS":
            tokens.append(Token(TokenKind.EQUALS, value, current_line))
            continue
        if kind_name == "IDENT":
            upper = value.upper()
            # Classifica solo se tutto uppercase (keyword GRS)
            if value == upper:
                kind = _classify_upper(value)
            elif value in _ATTR_KW:
                kind = TokenKind.ATTR_KW
            else:
                kind = TokenKind.IDENT
            tokens.append(Token(kind, value, current_line))
            continue

    tokens.append(Token(TokenKind.EOF, "", current_line))
    tokens = _merge_and_then(tokens)
    return tokens


def _merge_and_then(tokens: List[Token]) -> List[Token]:
    """
    Post-processing: sostituisce la coppia (KEYWORD:"AND", KEYWORD:"THEN")
    con un singolo token AND_THEN, ignorando NEWLINE/COMMENT intermedi.
    """
    result: List[Token] = []
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok.kind == TokenKind.KEYWORD and tok.value == "AND":
            # cerca il prossimo token significativo
            j = i + 1
            skipped: List[Token] = []
            while j < len(tokens) and tokens[j].kind in (
                TokenKind.NEWLINE, TokenKind.COMMENT
            ):
                skipped.append(tokens[j])
                j += 1
            if j < len(tokens) and tokens[j].kind == TokenKind.KEYWORD \
                    and tokens[j].value == "THEN":
                result.append(Token(TokenKind.AND_THEN, "AND THEN", tok.line))
                i = j + 1
                continue
            # non è AND THEN: emetti AND normale + gli skipped
            result.append(tok)
            result.extend(skipped)
            i = j
            continue
        result.append(tok)
        i += 1
    return result
