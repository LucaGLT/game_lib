"""
test_cli.py — Test di integrazione CLI (Fase 7).

Verifica il comportamento end-to-end di tutti i sottocomandi attraverso
build_parser() + func(args).  Non richiede subprocess: tutto in-process.

Esegui con:
    cd tools
    python -m pytest grs/tests/test_cli.py -v
"""

from __future__ import annotations

import io
import json
import sys
import os
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

from grs.cli import build_parser, main

# ---------------------------------------------------------------------------
# Percorsi costanti
# ---------------------------------------------------------------------------

_TOOLS_DIR   = Path(__file__).resolve().parent.parent.parent   # tools/
_EXAMPLE_GRS = (_TOOLS_DIR.parent / "gmRules" / "specs"
                / "turn-card-dungeon.example.grs")

# GRS minimale valido usato nei test sintetici
_MINIMAL_GRS = """\
@meta
game test
ns   test.ns
version 1.0.0
@end
@targets
T_Self :: ACTOR SELF required
@end
@conditions
C_A :: ACTOR_EXISTS(input.hero_id)
@end
@effects
E_Log :: MANUAL_EFFECT(test.event) [optional]
@end
@statuses
burned :: ONE_ONLY UNTIL_NEXT_TURN
    ON_APPLY ADD_TAG(T_Self, is_burned)
@end
@rules
R_Test [priority=10] ::
    IF C_A
    ON T_Self
    THEN E_Log?
@end
@triggers
Tr_Test [priority=1] ::
    ON_EVENT ACTION_SUBMITTED
    THEN E_Log?
@end
"""

# GRS con errore strutturale: regola senza target
_LINT_ERROR_GRS = """\
@effects
E :: DEAL_DAMAGE(T, 1)
@end
@rules
Bad_Rule :: THEN E
@end
"""

# GRS con errore semantico: riferimento undefined
_VALIDATE_ERROR_GRS = """\
@rules
R :: ON Undefined_Target THEN Undefined_Effect
@end
"""


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def _tmp_grs(content: str) -> str:
    """Scrive contenuto in un file .grs temporaneo e restituisce il percorso."""
    fd, path = tempfile.mkstemp(suffix=".grs")
    os.close(fd)
    Path(path).write_text(content, encoding="utf-8")
    return path


def _run(argv: list[str], *, capsys) -> tuple[int, str, str]:
    """
    Esegue il comando tramite build_parser().
    Restituisce (exit_code, stdout, stderr).
    """
    parser = build_parser()
    args   = parser.parse_args(argv)
    code   = args.func(args)
    out, err = capsys.readouterr()
    return code, out, err


# ---------------------------------------------------------------------------
# lint — OK
# ---------------------------------------------------------------------------

class TestLint:
    def test_lint_ok_text(self, capsys, tmp_path):
        f = tmp_path / "ok.grs"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, out, _ = _run(["lint", str(f)], capsys=capsys)
        assert code == 0
        assert "OK" in out

    def test_lint_error_exit1(self, capsys, tmp_path):
        f = tmp_path / "err.grs"
        f.write_text(_LINT_ERROR_GRS, encoding="utf-8")
        code, out, _ = _run(["lint", str(f)], capsys=capsys)
        assert code == 1
        assert "L-001" in out or "error" in out.lower()

    def test_lint_format_json(self, capsys, tmp_path):
        f = tmp_path / "err.grs"
        f.write_text(_LINT_ERROR_GRS, encoding="utf-8")
        code, out, _ = _run(["lint", str(f), "--format", "json"], capsys=capsys)
        assert code == 1
        # Estrae il blocco JSON (da '[' fino alla riga ']')
        start = out.index("[")
        end   = out.rindex("]") + 1
        data  = json.loads(out[start:end])
        assert isinstance(data, list)
        assert data[0]["severity"] == "ERROR"

    def test_lint_output_to_file(self, capsys, tmp_path):
        f   = tmp_path / "err.grs"
        out_file = tmp_path / "report.txt"
        f.write_text(_LINT_ERROR_GRS, encoding="utf-8")
        code, _, _ = _run(["lint", str(f), "-o", str(out_file)], capsys=capsys)
        assert code == 1
        assert out_file.exists()
        content = out_file.read_text(encoding="utf-8")
        assert "L-001" in content

    def test_lint_missing_file(self, capsys):
        parser = build_parser()
        args   = parser.parse_args(["lint", "/nonexistent/path.grs"])
        code   = args.func(args)
        assert code == 1


# ---------------------------------------------------------------------------
# validate — OK + errori + warning
# ---------------------------------------------------------------------------

class TestValidate:
    def test_validate_ok(self, capsys, tmp_path):
        f = tmp_path / "ok.grs"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, out, _ = _run(["validate", str(f)], capsys=capsys)
        # V-009 warnings su MANUAL_EFFECT [optional] prima di [stop] non ci sono nel minimal
        assert code == 0

    def test_validate_error_exit1(self, capsys, tmp_path):
        f = tmp_path / "err.grs"
        f.write_text(_VALIDATE_ERROR_GRS, encoding="utf-8")
        code, out, _ = _run(["validate", str(f)], capsys=capsys)
        assert code == 1
        assert "V-001" in out

    def test_validate_format_json(self, capsys, tmp_path):
        f = tmp_path / "err.grs"
        f.write_text(_VALIDATE_ERROR_GRS, encoding="utf-8")
        code, out, _ = _run(["validate", str(f), "--format", "json"], capsys=capsys)
        assert code == 1
        start = out.index("[")
        end   = out.rindex("]") + 1
        data  = json.loads(out[start:end])
        assert isinstance(data, list)

    def test_validate_output_to_file(self, capsys, tmp_path):
        f        = tmp_path / "err.grs"
        out_file = tmp_path / "val.txt"
        f.write_text(_VALIDATE_ERROR_GRS, encoding="utf-8")
        code, _, _ = _run(["validate", str(f), "-o", str(out_file)], capsys=capsys)
        assert code == 1
        assert out_file.exists()


# ---------------------------------------------------------------------------
# check — lint + validate
# ---------------------------------------------------------------------------

class TestCheck:
    def test_check_ok(self, capsys, tmp_path):
        f = tmp_path / "ok.grs"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, out, _ = _run(["check", str(f)], capsys=capsys)
        assert code == 0
        assert "OK" in out

    def test_check_combined_errors(self, capsys, tmp_path):
        # Contiene sia errori lint (L-001) che validate (V-001)
        combined = """\
@rules
Bad :: THEN Undef_Effect
@end
"""
        f = tmp_path / "combined.grs"
        f.write_text(combined, encoding="utf-8")
        code, out, _ = _run(["check", str(f)], capsys=capsys)
        assert code == 1

    def test_check_format_json(self, capsys, tmp_path):
        f = tmp_path / "err.grs"
        f.write_text(_LINT_ERROR_GRS, encoding="utf-8")
        code, out, _ = _run(["check", str(f), "--format", "json"], capsys=capsys)
        assert code == 1
        start = out.index("[")
        end   = out.rindex("]") + 1
        data  = json.loads(out[start:end])
        assert isinstance(data, list)


# ---------------------------------------------------------------------------
# yaml
# ---------------------------------------------------------------------------

class TestYaml:
    def test_yaml_stdout(self, capsys, tmp_path):
        f = tmp_path / "ok.grs"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, out, _ = _run(["yaml", str(f)], capsys=capsys)
        assert code == 0
        assert "meta:" in out
        assert "rules:" in out

    def test_yaml_to_file(self, capsys, tmp_path):
        f        = tmp_path / "ok.grs"
        out_file = tmp_path / "out.yaml"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, stdout, _ = _run(["yaml", str(f), "-o", str(out_file)], capsys=capsys)
        assert code == 0
        assert out_file.exists()
        content = out_file.read_text(encoding="utf-8")
        assert "meta:" in content
        assert "rules:" in content
        # Il testo stampato su stdout deve indicare il file di output
        assert str(out_file) in stdout or "→" in stdout

    def test_yaml_missing_file(self, capsys):
        parser = build_parser()
        args   = parser.parse_args(["yaml", "/nonexistent/path.grs"])
        code   = args.func(args)
        assert code == 1

    @pytest.mark.skipif(not _EXAMPLE_GRS.exists(),
                        reason="file di esempio non trovato")
    def test_yaml_smoke_example(self, capsys):
        code, out, _ = _run(["yaml", str(_EXAMPLE_GRS)], capsys=capsys)
        assert code == 0
        assert "meta:" in out
        assert "triggers:" in out


# ---------------------------------------------------------------------------
# grapho
# ---------------------------------------------------------------------------

class TestGrapho:
    def test_grapho_all_stdout(self, capsys, tmp_path):
        f = tmp_path / "ok.grs"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, out, _ = _run(["grapho", str(f)], capsys=capsys)
        assert code == 0
        assert "graph TD" in out
        assert "```mermaid" in out

    def test_grapho_rule_filter(self, capsys, tmp_path):
        f = tmp_path / "ok.grs"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, out, _ = _run(["grapho", str(f), "--rule", "R_Test"], capsys=capsys)
        assert code == 0
        assert "R_Test" in out
        assert "graph TD" in out

    def test_grapho_rule_not_found(self, capsys, tmp_path):
        f = tmp_path / "ok.grs"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, out, _ = _run(["grapho", str(f), "--rule", "Nonexistent"], capsys=capsys)
        # Il generatore emette un messaggio "not found" ma non fallisce con exit 1
        assert code == 0
        assert "not found" in out

    def test_grapho_to_file(self, capsys, tmp_path):
        f        = tmp_path / "ok.grs"
        out_file = tmp_path / "diagrams.md"
        f.write_text(_MINIMAL_GRS, encoding="utf-8")
        code, stdout, _ = _run(["grapho", str(f), "-o", str(out_file)], capsys=capsys)
        assert code == 0
        assert out_file.exists()
        content = out_file.read_text(encoding="utf-8")
        assert "graph TD" in content
        assert str(out_file) in stdout or "→" in stdout

    def test_grapho_missing_file(self, capsys):
        parser = build_parser()
        args   = parser.parse_args(["grapho", "/nonexistent/path.grs"])
        code   = args.func(args)
        assert code == 1

    @pytest.mark.skipif(not _EXAMPLE_GRS.exists(),
                        reason="file di esempio non trovato")
    def test_grapho_smoke_example_all(self, capsys):
        code, out, _ = _run(["grapho", str(_EXAMPLE_GRS)], capsys=capsys)
        assert code == 0
        assert "GRS Rule Graphs" in out
        assert "```mermaid" in out

    @pytest.mark.skipif(not _EXAMPLE_GRS.exists(),
                        reason="file di esempio non trovato")
    def test_grapho_smoke_example_rule(self, capsys):
        code, out, _ = _run(["grapho", str(_EXAMPLE_GRS), "--rule", "Base_Move"],
                            capsys=capsys)
        assert code == 0
        assert "Base_Move" in out
        assert "graph TD" in out


# ---------------------------------------------------------------------------
# Parser argparse: argomenti obbligatori e scelte valide
# ---------------------------------------------------------------------------

class TestParser:
    def test_no_subcommand_exits(self):
        parser = build_parser()
        with pytest.raises(SystemExit):
            parser.parse_args([])

    def test_invalid_format_exits(self, tmp_path):
        f = tmp_path / "x.grs"
        f.write_text("", encoding="utf-8")
        parser = build_parser()
        with pytest.raises(SystemExit):
            parser.parse_args(["lint", str(f), "--format", "xml"])

    def test_lint_default_format_is_text(self, tmp_path):
        f = tmp_path / "x.grs"
        f.write_text("", encoding="utf-8")
        parser = build_parser()
        args = parser.parse_args(["lint", str(f)])
        assert args.format == "text"

    def test_lint_json_format_parsed(self, tmp_path):
        f = tmp_path / "x.grs"
        f.write_text("", encoding="utf-8")
        parser = build_parser()
        args = parser.parse_args(["lint", str(f), "--format", "json"])
        assert args.format == "json"

    def test_yaml_no_output_is_none(self, tmp_path):
        f = tmp_path / "x.grs"
        f.write_text("", encoding="utf-8")
        parser = build_parser()
        args = parser.parse_args(["yaml", str(f)])
        assert args.output is None

    def test_grapho_no_rule_is_none(self, tmp_path):
        f = tmp_path / "x.grs"
        f.write_text("", encoding="utf-8")
        parser = build_parser()
        args = parser.parse_args(["grapho", str(f)])
        assert args.rule is None

    def test_all_subcommands_have_func(self, tmp_path):
        f = tmp_path / "x.grs"
        f.write_text("", encoding="utf-8")
        parser = build_parser()
        for cmd in ["lint", "validate", "check", "yaml", "grapho"]:
            args = parser.parse_args([cmd, str(f)])
            assert callable(args.func)
