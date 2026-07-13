#!/usr/bin/env python3
"""Debug script to inspect generated theme stylesheet."""

import sys
from pathlib import Path

# Setup paths
workspace_root = Path(__file__).parent
sys.path.insert(0, str(workspace_root / "pyLib"))

from PySide6.QtWidgets import QApplication
from gmGui.theme_manager import ThemeManager, _THEMES

def main() -> None:
    """Print all theme stylesheets and palettes."""
    app = QApplication([])
    manager = ThemeManager(app)
    
    for theme_id in ["scroll", "stone", "dark_moon", "blood", "techno"]:
        theme = _THEMES.get(theme_id)
        if theme is None:
            continue
        
        print(f"\n{'='*80}")
        print(f"THEME: {theme_id.upper()} ({theme.display_name})")
        print(f"{'='*80}")
        print(f"Colors: bg={theme.background}, panel={theme.panel}, text={theme.text}")
        print(f"        border={theme.border}, accent={theme.accent}")
        print(f"\nGenerated QSS:\n")
        print(manager._build_stylesheet(theme))
        print("\n" + "="*80 + "\n")

if __name__ == "__main__":
    main()
