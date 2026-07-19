"""Ensures ``gmWebServe`` is importable as a package when running pytest directly
from this ``tests`` folder (mirrors the ``sys.path`` bootstrap pattern already
used by ``GAME/Tic-Tac-Toe/WebApp/conftest.py``).
"""
import sys
from pathlib import Path

_PYLIB_DIR = Path(__file__).resolve().parents[2]
if str(_PYLIB_DIR) not in sys.path:
    sys.path.insert(0, str(_PYLIB_DIR))
