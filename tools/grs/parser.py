"""
parser.py — Parser ricorsivo discendente per file GRS v0.3.

Consuma la lista di Token prodotta da lexer.tokenize() e restituisce
un GrsDocument. In caso di errore sintattico solleva ParseError con
numero di riga.

Struttura:
    Parser
        parse()                  → GrsDocument
        _parse_meta()            → MetaBlock
        _parse_targets()         → List[TargetDef]
        _parse_conditions()      → List[ConditionDef]
        _parse_effects()         → List[EffectDef]
        _parse_statuses()        → List[StatusDef]
        _parse_rules()           → List[RuleDef]
        _parse_triggers()        → List[TriggerDef]
        _parse_cond_expr()       → CondExpr   (ricorsiva, gestisce AND/OR/NOT)
        _parse_effect_chain()    → List[EffectEntry]
        _parse_effect_call()     → EffectCall
"""

from __future__ import annotations
from typing import List, Optional

from .lexer import Token, TokenKind, tokenize, LexerError
from .ast_nodes import (
    GrsDocument, MetaBlock,
    TargetDef, ConditionDef, EffectDef, EffectCall, EffectEntry,
    StatusDef, StatusHook, RuleDef, TriggerDef,
    CondAtom, CondRef, CondNot, CondAnd, CondOr, CondExpr,
)


# ---------------------------------------------------------------------------
# ParseError
# ---------------------------------------------------------------------------

class ParseError(Exception):
    def __init__(self, message: str, line: int) -> None:
        super().__init__(f"[PARSE] line {line}: {message}")
        self.line = line


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------

class Parser:
    def __init__(self, tokens: List[Token]) -> None:
        # Filtra COMMENT e NEWLINE: il parser lavora su token significativi.
        # Conserva NEWLINE solo come separatore di blocco se necessario —
        # ma la grammatica GRS non è newline-sensitive; il parser usa
        # blocchi @xxx … @end come delimitatori.
        self._tokens: List[Token] = [
            t for t in tokens
            if t.kind not in (TokenKind.COMMENT, TokenKind.NEWLINE)
        ]
        self._pos: int = 0

    # -----------------------------------------------------------------------
    # Utilità di navigazione
    # -----------------------------------------------------------------------

    def _peek(self, offset: int = 0) -> Token:
        idx = self._pos + offset
        if idx >= len(self._tokens):
            return self._tokens[-1]  # EOF
        return self._tokens[idx]

    def _advance(self) -> Token:
        tok = self._tokens[self._pos]
        if self._pos < len(self._tokens) - 1:
            self._pos += 1
        return tok

    def _expect(self, kind: TokenKind, value: Optional[str] = None) -> Token:
        tok = self._peek()
        if tok.kind != kind:
            raise ParseError(
                f"atteso {kind.name!r}"
                + (f" ({value!r})" if value else "")
                + f", trovato {tok.kind.name!r} ({tok.value!r})",
                tok.line,
            )
        if value is not None and tok.value != value:
            raise ParseError(
                f"atteso {value!r}, trovato {tok.value!r}",
                tok.line,
            )
        return self._advance()

    def _match(self, kind: TokenKind, value: Optional[str] = None) -> bool:
        tok = self._peek()
        if tok.kind != kind:
            return False
        if value is not None and tok.value != value:
            return False
        return True

    def _consume_if(self, kind: TokenKind, value: Optional[str] = None) -> Optional[Token]:
        if self._match(kind, value):
            return self._advance()
        return None

    def _current_line(self) -> int:
        return self._peek().line

    # -----------------------------------------------------------------------
    # Entry point
    # -----------------------------------------------------------------------

    def parse(self) -> GrsDocument:
        doc = GrsDocument(meta=None)
        while not self._match(TokenKind.EOF):
            if not self._match(TokenKind.BLOCK_OPEN):
                raise ParseError(
                    f"atteso blocco @xxx, trovato {self._peek().kind.name!r} ({self._peek().value!r})",
                    self._current_line(),
                )
            block_tok = self._advance()
            name = block_tok.value  # es. "@meta"

            if name == "@meta":
                doc.meta = self._parse_meta(block_tok.line)
            elif name == "@targets":
                doc.targets = self._parse_targets()
            elif name == "@conditions":
                doc.conditions = self._parse_conditions()
            elif name == "@effects":
                doc.effects = self._parse_effects()
            elif name == "@statuses":
                doc.statuses = self._parse_statuses()
            elif name == "@rules":
                doc.rules = self._parse_rules()
            elif name == "@triggers":
                doc.triggers = self._parse_triggers()
            else:
                raise ParseError(f"blocco sconosciuto: {name!r}", block_tok.line)

        return doc

    # -----------------------------------------------------------------------
    # @meta
    # -----------------------------------------------------------------------

    def _parse_meta(self, start_line: int) -> MetaBlock:
        fields: dict = {}
        while not self._match(TokenKind.BLOCK_END):
            if self._match(TokenKind.EOF):
                raise ParseError("@meta non chiuso con @end", start_line)
            key_tok = self._advance()
            # il valore segue immediatamente sullo stesso "segmento"
            val_tok = self._advance()
            fields[key_tok.value] = val_tok.value
        self._expect(TokenKind.BLOCK_END)
        return MetaBlock(
            game_id=fields.get("game", ""),
            namespace=fields.get("ns", ""),
            version=fields.get("version", ""),
            min_gmrules=fields.get("min_gmrules"),
            line=start_line,
        )

    # -----------------------------------------------------------------------
    # @targets
    # -----------------------------------------------------------------------

    def _parse_targets(self) -> List[TargetDef]:
        targets: List[TargetDef] = []
        while not self._match(TokenKind.BLOCK_END):
            if self._match(TokenKind.EOF):
                raise ParseError("@targets non chiuso con @end", self._current_line())
            targets.append(self._parse_target_line())
        self._expect(TokenKind.BLOCK_END)
        return targets

    def _parse_target_line(self) -> TargetDef:
        name_tok = self._advance()   # nome target
        line = name_tok.line
        self._expect(TokenKind.DEFINE)

        # TargetKind: ACTOR, LOCATION, CARD, DECK, ...
        # LOCATION può comparire sia come TARGET_KIND che come TARGET_SEL
        kind_tok = self._advance()
        kind = kind_tok.value

        # TargetSelector
        sel_tok = self._advance()
        selector = sel_tok.value

        range_type: Optional[str] = None
        range_n: Optional[int] = None
        required = True
        allow_self = True
        needs_tags: List[str] = []
        forbids_tags: List[str] = []

        # Attributi opzionali (ordine libero ma la spec li descrive in sequenza)
        while True:
            tok = self._peek()

            if tok.kind == TokenKind.ATTR_KW and tok.value == "range":
                self._advance()
                rt_tok = self._advance()
                range_type = rt_tok.value
                if range_type == "WITHIN_N_LOCATIONS":
                    n_tok = self._expect(TokenKind.INTEGER)
                    range_n = int(n_tok.value)

            elif tok.kind == TokenKind.ATTR_KW and tok.value == "required":
                self._advance()
                required = True

            elif tok.kind == TokenKind.ATTR_KW and tok.value == "optional":
                self._advance()
                required = False

            elif tok.kind == TokenKind.ATTR_KW and tok.value == "no_self":
                self._advance()
                allow_self = False

            elif tok.kind == TokenKind.ATTR_KW and tok.value == "needs":
                self._advance()
                needs_tags = self._parse_tag_list()

            elif tok.kind == TokenKind.ATTR_KW and tok.value == "forbids":
                self._advance()
                forbids_tags = self._parse_tag_list()

            else:
                break

        return TargetDef(
            name=name_tok.value,
            kind=kind,
            selector=selector,
            range_type=range_type,
            range_n=range_n,
            required=required,
            allow_self=allow_self,
            needs_tags=needs_tags,
            forbids_tags=forbids_tags,
            line=line,
        )

    def _parse_tag_list(self) -> List[str]:
        """Legge una lista di tag separati da virgola (senza spazi tra tag)."""
        first = self._advance()
        # i tag possono essere IDENT o avere lettere e underscore
        tags = [t.strip() for t in first.value.split(",") if t.strip()]
        while self._peek().kind == TokenKind.COMMA:
            self._advance()
            nxt = self._advance()
            tags.extend(t.strip() for t in nxt.value.split(",") if t.strip())
        return tags

    # -----------------------------------------------------------------------
    # @conditions
    # -----------------------------------------------------------------------

    def _parse_conditions(self) -> List[ConditionDef]:
        defs: List[ConditionDef] = []
        while not self._match(TokenKind.BLOCK_END):
            if self._match(TokenKind.EOF):
                raise ParseError("@conditions non chiuso con @end", self._current_line())
            defs.append(self._parse_condition_line())
        self._expect(TokenKind.BLOCK_END)
        return defs

    def _parse_condition_line(self) -> ConditionDef:
        name_tok = self._advance()
        line = name_tok.line
        self._expect(TokenKind.DEFINE)
        expr = self._parse_cond_expr()
        return ConditionDef(name=name_tok.value, expr=expr, line=line)

    # CondExpr parser: precedenza OR < AND < NOT < atom
    # Ricorsione discendente classica.

    def _parse_cond_expr(self) -> CondExpr:
        return self._parse_cond_or()

    def _parse_cond_or(self) -> CondExpr:
        left = self._parse_cond_and()
        while self._match(TokenKind.KEYWORD, "OR"):
            line = self._peek().line
            self._advance()
            right = self._parse_cond_and()
            left = CondOr(left=left, right=right, line=line)
        return left

    def _parse_cond_and(self) -> CondExpr:
        left = self._parse_cond_not()
        while self._match(TokenKind.KEYWORD, "AND"):
            line = self._peek().line
            self._advance()
            right = self._parse_cond_not()
            left = CondAnd(left=left, right=right, line=line)
        return left

    def _parse_cond_not(self) -> CondExpr:
        if self._match(TokenKind.KEYWORD, "NOT"):
            line = self._peek().line
            self._advance()
            operand = self._parse_cond_atom()
            return CondNot(operand=operand, line=line)
        return self._parse_cond_atom()

    def _parse_cond_atom(self) -> CondExpr:
        tok = self._peek()

        # Parentesi aperta: espressione raggruppata
        if tok.kind == TokenKind.LPAREN:
            self._advance()
            expr = self._parse_cond_expr()
            self._expect(TokenKind.RPAREN)
            return expr

        # Condizione atomica tipo: ACTOR_EXISTS(...)
        if tok.kind == TokenKind.COND_TYPE:
            return self._parse_cond_atom_call()

        # Riferimento a condizione nominata (IDENT)
        if tok.kind in (TokenKind.IDENT,):
            self._advance()
            return CondRef(name=tok.value, line=tok.line)

        raise ParseError(
            f"attesa condizione o nome, trovato {tok.kind.name!r} ({tok.value!r})",
            tok.line,
        )

    def _parse_cond_atom_call(self) -> CondAtom:
        type_tok = self._advance()
        line = type_tok.line
        args: List[str] = []
        if self._match(TokenKind.LPAREN):
            self._advance()
            # argomenti fino a )
            while not self._match(TokenKind.RPAREN):
                if self._match(TokenKind.EOF):
                    raise ParseError("argomenti condizione non chiusi con )", line)
                arg_tok = self._advance()
                args.append(arg_tok.value)
                self._consume_if(TokenKind.COMMA)
            self._expect(TokenKind.RPAREN)
        return CondAtom(cond_type=type_tok.value, args=args, line=line)

    # -----------------------------------------------------------------------
    # @effects
    # -----------------------------------------------------------------------

    def _parse_effects(self) -> List[EffectDef]:
        defs: List[EffectDef] = []
        while not self._match(TokenKind.BLOCK_END):
            if self._match(TokenKind.EOF):
                raise ParseError("@effects non chiuso con @end", self._current_line())
            defs.append(self._parse_effect_def())
        self._expect(TokenKind.BLOCK_END)
        return defs

    def _parse_effect_def(self) -> EffectDef:
        name_tok = self._advance()
        line = name_tok.line
        self._expect(TokenKind.DEFINE)
        call = self._parse_effect_call(line)
        return EffectDef(name=name_tok.value, call=call, line=line)

    def _parse_effect_call(self, line: int) -> EffectCall:
        type_tok = self._advance()  # es. MOVE_ACTOR
        target_ref: Optional[str] = None
        args: List[str] = []
        optional = False
        stop_on_failure = True  # default implicito nella spec

        if self._match(TokenKind.LPAREN):
            self._advance()
            # primo argomento: target_ref (assente per MANUAL_EFFECT)
            if not self._match(TokenKind.RPAREN):
                first = self._advance()
                if type_tok.value != "MANUAL_EFFECT":
                    target_ref = first.value
                else:
                    # MANUAL_EFFECT(event_name) — nessun target, event è il primo arg
                    args.append(first.value)
                # argomenti aggiuntivi
                while self._match(TokenKind.COMMA):
                    self._advance()
                    arg_tok = self._advance()
                    args.append(arg_tok.value)
            self._expect(TokenKind.RPAREN)

        # Modificatori: [optional], [stop], [continue]
        while self._match(TokenKind.MODIFIER):
            mod = self._advance().value
            if mod == "[optional]":
                optional = True
                stop_on_failure = False
            elif mod == "[stop]":
                stop_on_failure = True
            elif mod == "[continue]":
                stop_on_failure = False

        return EffectCall(
            effect_type=type_tok.value,
            target_ref=target_ref,
            args=args,
            optional=optional,
            stop_on_failure=stop_on_failure,
            line=line,
        )

    # Effect chain: E_Foo AND THEN E_Bar AND THEN ...
    # Ogni elemento è un EffectEntry (nome riferito oppure chiamata inline).

    def _parse_effect_chain(self) -> List[EffectEntry]:
        entries: List[EffectEntry] = []
        entries.append(self._parse_effect_entry())
        while self._match(TokenKind.AND_THEN):
            self._advance()
            entries.append(self._parse_effect_entry())
        return entries

    def _parse_effect_entry(self) -> EffectEntry:
        tok = self._peek()
        line = tok.line
        optional = False
        call: Optional[EffectCall] = None
        name: Optional[str] = None

        # Chiamata inline: tipo effetto direttamente
        if tok.kind == TokenKind.EFFECT_TYPE:
            call = self._parse_effect_call(line)
        else:
            # Riferimento a effetto nominato
            name_tok = self._advance()
            name = name_tok.value
            # suffisso ? = optional
            if self._match(TokenKind.QUESTION):
                self._advance()
                optional = True
            elif self._match(TokenKind.BANG):
                self._advance()
                # stop_on_failure implicito, nessun flag aggiuntivo necessario

        return EffectEntry(name=name, call=call, optional=optional, line=line)

    # -----------------------------------------------------------------------
    # @statuses
    # -----------------------------------------------------------------------

    def _parse_statuses(self) -> List[StatusDef]:
        defs: List[StatusDef] = []
        while not self._match(TokenKind.BLOCK_END):
            if self._match(TokenKind.EOF):
                raise ParseError("@statuses non chiuso con @end", self._current_line())
            defs.append(self._parse_status_def())
        self._expect(TokenKind.BLOCK_END)
        return defs

    def _parse_status_def(self) -> StatusDef:
        name_tok = self._advance()
        line = name_tok.line
        self._expect(TokenKind.DEFINE)

        stacking_tok = self._advance()   # ONE_ONLY, REFRESH, ...
        duration_tok = self._advance()   # PERMANENT, FOR_N, ...

        amount: Optional[int] = None
        value_str: Optional[str] = None

        # attributi opzionali dopo DurationType
        if self._match(TokenKind.ATTR_KW, "amount"):
            self._advance()
            n_tok = self._expect(TokenKind.INTEGER)
            amount = int(n_tok.value)

        if self._match(TokenKind.ATTR_KW, "value"):
            self._advance()
            v_tok = self._advance()
            value_str = v_tok.value

        hooks: List[StatusHook] = []
        while self._match(TokenKind.HOOK_KW):
            hooks.append(self._parse_status_hook())

        return StatusDef(
            name=name_tok.value,
            stacking_mode=stacking_tok.value,
            duration_type=duration_tok.value,
            amount=amount,
            value=value_str,
            hooks=hooks,
            line=line,
        )

    def _parse_status_hook(self) -> StatusHook:
        hook_tok = self._advance()   # ON_APPLY, ON_REMOVE, ...
        line = hook_tok.line
        chain = self._parse_effect_chain()
        return StatusHook(hook_type=hook_tok.value, chain=chain, line=line)

    # -----------------------------------------------------------------------
    # @rules
    # -----------------------------------------------------------------------

    def _parse_rules(self) -> List[RuleDef]:
        defs: List[RuleDef] = []
        while not self._match(TokenKind.BLOCK_END):
            if self._match(TokenKind.EOF):
                raise ParseError("@rules non chiuso con @end", self._current_line())
            defs.append(self._parse_rule_def())
        self._expect(TokenKind.BLOCK_END)
        return defs

    def _parse_rule_def(self) -> RuleDef:
        name_tok = self._advance()
        line = name_tok.line
        priority = 0
        enabled = True

        # attributi opzionali: [priority=N] [disabled]
        while self._match(TokenKind.BRACKET_OPEN):
            self._advance()  # [
            attr_tok = self._advance()
            if attr_tok.kind == TokenKind.ATTR_KW and attr_tok.value == "priority":
                self._expect(TokenKind.EQUALS)
                n_tok = self._expect(TokenKind.INTEGER)
                priority = int(n_tok.value)
            elif attr_tok.kind == TokenKind.ATTR_KW and attr_tok.value == "disabled":
                enabled = False
            self._expect(TokenKind.BRACKET_CLOSE)

        self._expect(TokenKind.DEFINE)

        condition: Optional[CondExpr] = None
        target: str = ""

        # IF e ON possono arrivare in ordine qualsiasi
        for _ in range(2):
            tok = self._peek()
            if tok.kind == TokenKind.KEYWORD and tok.value == "IF":
                self._advance()
                condition = self._parse_cond_expr()
            elif tok.kind == TokenKind.KEYWORD and tok.value == "ON":
                self._advance()
                target_tok = self._advance()
                target = target_tok.value

        # THEN <effect chain>
        self._expect(TokenKind.KEYWORD, "THEN")
        effects = self._parse_effect_chain()

        return RuleDef(
            name=name_tok.value,
            priority=priority,
            enabled=enabled,
            condition=condition,
            target=target,
            effects=effects,
            line=line,
        )

    # -----------------------------------------------------------------------
    # @triggers
    # -----------------------------------------------------------------------

    def _parse_triggers(self) -> List[TriggerDef]:
        defs: List[TriggerDef] = []
        while not self._match(TokenKind.BLOCK_END):
            if self._match(TokenKind.EOF):
                raise ParseError("@triggers non chiuso con @end", self._current_line())
            defs.append(self._parse_trigger_def())
        self._expect(TokenKind.BLOCK_END)
        return defs

    def _parse_trigger_def(self) -> TriggerDef:
        name_tok = self._advance()
        line = name_tok.line
        priority = 0
        enabled = True

        while self._match(TokenKind.BRACKET_OPEN):
            self._advance()
            attr_tok = self._advance()
            if attr_tok.kind == TokenKind.ATTR_KW and attr_tok.value == "priority":
                self._expect(TokenKind.EQUALS)
                n_tok = self._expect(TokenKind.INTEGER)
                priority = int(n_tok.value)
            elif attr_tok.kind == TokenKind.ATTR_KW and attr_tok.value == "disabled":
                enabled = False
            self._expect(TokenKind.BRACKET_CLOSE)

        self._expect(TokenKind.DEFINE)
        self._expect(TokenKind.EVENT_KW)          # ON_EVENT
        event_tok = self._advance()               # tipo evento
        event_type = event_tok.value

        condition: Optional[CondExpr] = None
        if self._match(TokenKind.KEYWORD, "IF"):
            self._advance()
            condition = self._parse_cond_expr()

        self._expect(TokenKind.KEYWORD, "THEN")
        effects = self._parse_effect_chain()

        return TriggerDef(
            name=name_tok.value,
            priority=priority,
            enabled=enabled,
            event_type=event_type,
            condition=condition,
            effects=effects,
            line=line,
        )


# ---------------------------------------------------------------------------
# Funzione di convenienza
# ---------------------------------------------------------------------------

def parse_file(path: str) -> GrsDocument:
    """Legge un file .grs e restituisce il GrsDocument."""
    with open(path, encoding="utf-8") as fh:
        source = fh.read()
    tokens = tokenize(source)
    return Parser(tokens).parse()


def parse_source(source: str) -> GrsDocument:
    """Parsa una stringa sorgente GRS e restituisce il GrsDocument."""
    tokens = tokenize(source)
    return Parser(tokens).parse()
