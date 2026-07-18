"""Ensures ``eng_serve`` is importable as a package when running pytest.

``GAME/Tic-Tac-Toe`` contains hyphens, so this ``WebApp`` folder cannot be
part of a dotted import path — instead we add this directory itself to
``sys.path`` so that ``import eng_serve`` resolves directly (same pattern
used by ``GAME/Tic-Tac-Toe/GUI/main.py`` for its own ``app``/``widgets``
packages).
"""
import sys
from pathlib import Path

_WEBAPP_DIR = Path(__file__).resolve().parent
if str(_WEBAPP_DIR) not in sys.path:
    sys.path.insert(0, str(_WEBAPP_DIR))
