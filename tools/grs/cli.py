"""
cli.py — Interfaccia a riga di comando per il tool GRS.

Sottocomandi disponibili:
    lint      <file.grs>   Controlli strutturali (L-xxx)
    validate  <file.grs>   Validazioni semantiche (V-xxx)
    check     <file.grs>   lint + validate insieme
    yaml      <file.grs>   (stub) Genera YAML canonico
    grapho    <file.grs>   (stub) Genera diagramma Mermaid
"""

from __future__ import annotations
import argparse
import sys
from pathlib import Path

from .parser import parse_file, ParseError
from .lexer import LexerError
from .linter import lint
from .validator import validate
from .diagnostic import format_text, format_json, has_errors, Diagnostic
from typing import List


# ---------------------------------------------------------------------------
# Utilità comuni
# ---------------------------------------------------------------------------

def _parse_or_exit(path: str):
    """Parsa il file GRS; in caso di errore stampa e ritorna None."""
    if not Path(path).is_file():
        print(f"[ERROR] file non trovato: {path}", file=sys.stderr)
        return None
    try:
        return parse_file(path)
    except (LexerError, ParseError) as exc:
        print(str(exc), file=sys.stderr)
        return None


def _print_summary(doc, path: str) -> None:
    print(f"Parsed  {path}")
    if doc.meta:
        print(f"  game={doc.meta.game_id}  ns={doc.meta.namespace}"
              f"  v={doc.meta.version}")
    print(f"  targets={len(doc.targets)}  conditions={len(doc.conditions)}"
          f"  effects={len(doc.effects)}  statuses={len(doc.statuses)}"
          f"  rules={len(doc.rules)}  triggers={len(doc.triggers)}")


def _emit(diags: List[Diagnostic], fmt: str, output: str) -> None:
    """Stampa o scrive i diagnostici nel formato richiesto."""
    text = format_json(diags) if fmt == "json" else format_text(diags)
    if not text:
        return
    if output:
        Path(output).write_text(text, encoding="utf-8")
        print(f"  → {output}")
    else:
        print(text)


def _exit_code(diags: List[Diagnostic]) -> int:
    return 1 if has_errors(diags) else 0


# ---------------------------------------------------------------------------
# Sottocomandi
# ---------------------------------------------------------------------------

def _cmd_lint(args: argparse.Namespace) -> int:
    doc = _parse_or_exit(args.file)
    if doc is None:
        return 1
    _print_summary(doc, args.file)
    diags = lint(doc)
    if not diags:
        print("  lint: OK — nessun problema strutturale")
    else:
        _emit(diags, getattr(args, "format", "text"), getattr(args, "output", None))
        errors = sum(1 for d in diags if d.severity == "ERROR")
        warns = sum(1 for d in diags if d.severity == "WARNING")
        print(f"  lint: {errors} error(i), {warns} warning(s)")
    return _exit_code(diags)


def _cmd_validate(args: argparse.Namespace) -> int:
    doc = _parse_or_exit(args.file)
    if doc is None:
        return 1
    _print_summary(doc, args.file)
    diags = validate(doc)
    if not diags:
        print("  validate: OK — nessun problema semantico")
    else:
        _emit(diags, getattr(args, "format", "text"), getattr(args, "output", None))
        errors = sum(1 for d in diags if d.severity == "ERROR")
        warns = sum(1 for d in diags if d.severity == "WARNING")
        print(f"  validate: {errors} error(i), {warns} warning(s)")
    return _exit_code(diags)


def _cmd_check(args: argparse.Namespace) -> int:
    doc = _parse_or_exit(args.file)
    if doc is None:
        return 1
    _print_summary(doc, args.file)
    diags = lint(doc) + validate(doc)
    diags.sort(key=lambda d: d.line)
    if not diags:
        print("  check: OK — nessun problema")
    else:
        _emit(diags, getattr(args, "format", "text"), getattr(args, "output", None))
        errors = sum(1 for d in diags if d.severity == "ERROR")
        warns = sum(1 for d in diags if d.severity == "WARNING")
        print(f"  check: {errors} error(i), {warns} warning(s)")
    return _exit_code(diags)


def _cmd_yaml(args: argparse.Namespace) -> int:
    print("[INFO] sottocomando 'yaml' non ancora implementato (Fase 5)")
    return 0


def _cmd_grapho(args: argparse.Namespace) -> int:
    print("[INFO] sottocomando 'grapho' non ancora implementato (Fase 6)")
    return 0


# ---------------------------------------------------------------------------
# Costruzione parser argparse
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(
        prog="grs",
        description="Tool per file GRS — Game Rule Script v0.3",
    )
    sub = root.add_subparsers(dest="command", required=True)

    # lint
    p_lint = sub.add_parser("lint", help="Controlli strutturali (L-xxx)")
    p_lint.add_argument("file", help="Percorso del file .grs")
    p_lint.add_argument("-o", "--output", help="Salva output su file")
    p_lint.add_argument("--format", choices=["text", "json"], default="text")
    p_lint.set_defaults(func=_cmd_lint)

    # validate
    p_val = sub.add_parser("validate", help="Validazioni semantiche (V-xxx)")
    p_val.add_argument("file", help="Percorso del file .grs")
    p_val.add_argument("-o", "--output", help="Salva output su file")
    p_val.add_argument("--format", choices=["text", "json"], default="text")
    p_val.set_defaults(func=_cmd_validate)

    # check
    p_check = sub.add_parser("check", help="lint + validate (tutti i controlli)")
    p_check.add_argument("file", help="Percorso del file .grs")
    p_check.add_argument("-o", "--output", help="Salva output su file")
    p_check.add_argument("--format", choices=["text", "json"], default="text")
    p_check.set_defaults(func=_cmd_check)

    # yaml
    p_yaml = sub.add_parser("yaml", help="Genera YAML canonico da un file .grs")
    p_yaml.add_argument("file", help="Percorso del file .grs")
    p_yaml.add_argument("-o", "--output", help="File di output .yaml")
    p_yaml.set_defaults(func=_cmd_yaml)

    # grapho
    p_graph = sub.add_parser("grapho", help="Genera diagramma Mermaid da un file .grs")
    p_graph.add_argument("file", help="Percorso del file .grs")
    p_graph.add_argument("-o", "--output", help="File di output .md")
    p_graph.add_argument("--rule", help="Filtra il grafo su una sola regola (es. Base_Move)")
    p_graph.set_defaults(func=_cmd_grapho)

    return root


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    sys.exit(args.func(args))
