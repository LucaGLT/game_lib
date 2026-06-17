"""
diagnostic.py — Dataclass Diagnostic e funzioni di formattazione.

Un Diagnostic porta severity ("ERROR" | "WARNING"), numero di riga,
codice mnemonico (es. "V-001") e messaggio leggibile.
"""

from __future__ import annotations
import json
from dataclasses import dataclass
from typing import List


@dataclass
class Diagnostic:
    severity: str   # "ERROR" | "WARNING"
    line: int
    code: str
    message: str

    def __str__(self) -> str:
        return f"[{self.severity:<7}] line {self.line:>4} [{self.code}] {self.message}"


def format_text(diagnostics: List[Diagnostic]) -> str:
    """Formatta la lista come testo leggibile, ordinata per riga."""
    if not diagnostics:
        return ""
    return "\n".join(str(d) for d in sorted(diagnostics, key=lambda d: d.line))


def format_json(diagnostics: List[Diagnostic]) -> str:
    """Formatta la lista come array JSON, ordinata per riga."""
    data = [
        {
            "severity": d.severity,
            "line": d.line,
            "code": d.code,
            "message": d.message,
        }
        for d in sorted(diagnostics, key=lambda d: d.line)
    ]
    return json.dumps(data, indent=2)


def has_errors(diagnostics: List[Diagnostic]) -> bool:
    return any(d.severity == "ERROR" for d in diagnostics)
