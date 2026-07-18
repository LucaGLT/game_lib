"""Ensures ``eng_serve`` is importable as a package when running pytest.

Same pattern as ``GAME/Tic-Tac-Toe/WebApp/conftest.py``: this ``WebApp``
folder is added directly to ``sys.path`` so ``import eng_serve`` resolves.
"""
import sys
from pathlib import Path

_WEBAPP_DIR = Path(__file__).resolve().parent
if str(_WEBAPP_DIR) not in sys.path:
    sys.path.insert(0, str(_WEBAPP_DIR))
