"""
cli.py — Interfaccia a riga di comando per il tool GRS.

Sottocomandi disponibili:
    lint    <file.grs>   Valida la sintassi del file
    yaml    <file.grs>   (stub) Genera YAML canonico
    grapho  <file.grs>   (stub) Genera diagramma Mermaid
    check   <file.grs>   Alias: lint + validate insieme
"""

from __future__ import annotations
import argparse
import sys
from pathlib import Path

from .parser import parse_file, ParseError
from .lexer import LexerError


def _cmd_lint(args: argparse.Namespace) -> int:
    path = args.file
    if not Path(path).is_file():
        print(f"[ERROR] file non trovato: {path}", file=sys.stderr)
        return 1
    try:
        doc = parse_file(path)
    except (LexerError, ParseError) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    # Riepilogo struttura
    print(f"OK  {path}")
    if doc.meta:
        print(f"    game={doc.meta.game_id}  ns={doc.meta.namespace}  v={doc.meta.version}")
    print(f"    targets={len(doc.targets)}  conditions={len(doc.conditions)}"
          f"  effects={len(doc.effects)}  statuses={len(doc.statuses)}"
          f"  rules={len(doc.rules)}  triggers={len(doc.triggers)}")
    return 0


def _cmd_yaml(args: argparse.Namespace) -> int:
    print("[INFO] sottocomando 'yaml' non ancora implementato (Fase 5)")
    return 0


def _cmd_grapho(args: argparse.Namespace) -> int:
    print("[INFO] sottocomando 'grapho' non ancora implementato (Fase 6)")
    return 0


def _cmd_check(args: argparse.Namespace) -> int:
    rc = _cmd_lint(args)
    # validate verrà aggiunto in Fase 4
    return rc


def build_parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(
        prog="grs",
        description="Tool per file GRS — Game Rule Script v0.3",
    )
    sub = root.add_subparsers(dest="command", required=True)

    # lint
    p_lint = sub.add_parser("lint", help="Valida la sintassi di un file .grs")
    p_lint.add_argument("file", help="Percorso del file .grs")
    p_lint.add_argument("-o", "--output", help="File di output (non usato da lint)")
    p_lint.set_defaults(func=_cmd_lint)

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

    # check
    p_check = sub.add_parser("check", help="lint + validate (alias)")
    p_check.add_argument("file", help="Percorso del file .grs")
    p_check.add_argument("--format", choices=["text", "json"], default="text")
    p_check.set_defaults(func=_cmd_check)

    return root


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    sys.exit(args.func(args))
