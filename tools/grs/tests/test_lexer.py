"""
test_lexer.py — Unit test per lexer.py

Esegui con:
    cd tools
    python -m pytest grs/tests/test_lexer.py -v
"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import pytest
from grs.lexer import tokenize, TokenKind, Token


def kinds(source: str):
    """Restituisce solo i TokenKind significativi (no EOF)."""
    return [t.kind for t in tokenize(source) if t.kind != TokenKind.EOF]


def values(source: str):
    return [(t.kind, t.value) for t in tokenize(source) if t.kind != TokenKind.EOF]


# ---------------------------------------------------------------------------
# Commenti
# ---------------------------------------------------------------------------

def test_comment_solo():
    toks = [t for t in tokenize("# questo è un commento") if t.kind != TokenKind.EOF]
    assert len(toks) == 1
    assert toks[0].kind == TokenKind.COMMENT


def test_comment_inline():
    toks = [t for t in tokenize("ACTOR SELF # commento") if t.kind != TokenKind.EOF]
    kinds_ = [t.kind for t in toks if t.kind != TokenKind.NEWLINE]
    assert TokenKind.TARGET_KIND in kinds_
    assert TokenKind.TARGET_SEL in kinds_
    assert TokenKind.COMMENT in kinds_


# ---------------------------------------------------------------------------
# Block open / end
# ---------------------------------------------------------------------------

def test_block_open():
    for name in ["@meta", "@targets", "@conditions", "@effects",
                 "@statuses", "@rules", "@triggers"]:
        toks = [t for t in tokenize(name) if t.kind != TokenKind.EOF]
        assert toks[0].kind == TokenKind.BLOCK_OPEN
        assert toks[0].value == name


def test_block_end():
    toks = [t for t in tokenize("@end") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.BLOCK_END


# ---------------------------------------------------------------------------
# DEFINE ::
# ---------------------------------------------------------------------------

def test_define():
    toks = [t for t in tokenize("Name :: REST") if t.kind not in (TokenKind.EOF, TokenKind.NEWLINE)]
    assert any(t.kind == TokenKind.DEFINE for t in toks)


# ---------------------------------------------------------------------------
# Keywords
# ---------------------------------------------------------------------------

def test_keywords():
    for kw in ["IF", "ON", "THEN", "AND", "OR", "NOT"]:
        toks = [t for t in tokenize(kw) if t.kind != TokenKind.EOF]
        assert toks[0].kind == TokenKind.KEYWORD
        assert toks[0].value == kw


# ---------------------------------------------------------------------------
# AND THEN merge
# ---------------------------------------------------------------------------

def test_and_then_merge():
    toks = [t for t in tokenize("THEN E_Move AND THEN E_Log")
            if t.kind not in (TokenKind.EOF, TokenKind.NEWLINE)]
    ks = [t.kind for t in toks]
    assert TokenKind.AND_THEN in ks
    # AND separato non deve sopravvivere
    and_count = sum(1 for t in toks if t.kind == TokenKind.KEYWORD and t.value == "AND")
    assert and_count == 0


def test_and_without_then_not_merged():
    toks = [t for t in tokenize("C_A AND C_B")
            if t.kind not in (TokenKind.EOF, TokenKind.NEWLINE)]
    ks = [t.kind for t in toks]
    assert TokenKind.AND_THEN not in ks
    assert TokenKind.KEYWORD in ks


# ---------------------------------------------------------------------------
# Runtime refs
# ---------------------------------------------------------------------------

def test_ref_input():
    toks = [t for t in tokenize("input.hero_id") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.REF
    assert toks[0].value == "input.hero_id"


def test_ref_event():
    toks = [t for t in tokenize("event.actor_id") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.REF


# ---------------------------------------------------------------------------
# Modifier brackets
# ---------------------------------------------------------------------------

def test_modifiers():
    for mod in ["[optional]", "[stop]", "[continue]"]:
        toks = [t for t in tokenize(mod) if t.kind != TokenKind.EOF]
        assert toks[0].kind == TokenKind.MODIFIER
        assert toks[0].value == mod


# ---------------------------------------------------------------------------
# TargetKind / TargetSelector
# ---------------------------------------------------------------------------

def test_target_kind_actor():
    toks = [t for t in tokenize("ACTOR") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.TARGET_KIND


def test_target_selector_self():
    toks = [t for t in tokenize("SELF") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.TARGET_SEL


# ---------------------------------------------------------------------------
# EffectType / CondType
# ---------------------------------------------------------------------------

def test_effect_type():
    toks = [t for t in tokenize("DEAL_DAMAGE") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.EFFECT_TYPE


def test_cond_type():
    toks = [t for t in tokenize("ACTOR_EXISTS") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.COND_TYPE


# ---------------------------------------------------------------------------
# INTEGER
# ---------------------------------------------------------------------------

def test_integer_positive():
    toks = [t for t in tokenize("42") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.INTEGER
    assert toks[0].value == "42"


def test_integer_negative():
    toks = [t for t in tokenize("-3") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.INTEGER


# ---------------------------------------------------------------------------
# Numeri di riga
# ---------------------------------------------------------------------------

def test_line_tracking():
    source = "@meta\ngame test\n@end"
    toks = tokenize(source)
    block = next(t for t in toks if t.kind == TokenKind.BLOCK_OPEN)
    end = next(t for t in toks if t.kind == TokenKind.BLOCK_END)
    assert block.line == 1
    assert end.line == 3


# ---------------------------------------------------------------------------
# IDENT per nomi dotted (dungeon.turn.started)
# ---------------------------------------------------------------------------

def test_dotted_ident():
    toks = [t for t in tokenize("dungeon.turn.started") if t.kind != TokenKind.EOF]
    assert toks[0].kind == TokenKind.IDENT
    assert toks[0].value == "dungeon.turn.started"
