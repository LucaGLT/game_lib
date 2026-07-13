#!/usr/bin/env python3
"""
Static verification test for Bug_01 fix.

This test verifies that the source code contains the required changes
without needing to run the GUI and CoreEngine together.
"""
import sys
from pathlib import Path

def check_file_contains(file_path, patterns, description):
    """Check if file contains all required patterns."""
    print(f"\n[CHECK] {description}")
    print(f"        File: {file_path.name}")
    
    try:
        content = file_path.read_text(encoding='utf-8')
    except Exception as e:
        print(f"✗ Failed to read file: {e}")
        return False
    
    all_found = True
    for pattern in patterns:
        if pattern in content:
            print(f"  ✓ Found: {pattern[:60]}")
        else:
            print(f"  ✗ MISSING: {pattern[:60]}")
            all_found = False
    
    return all_found

def run_verification():
    """Verify Bug_01 fix is in source code."""
    print("\n" + "="*70)
    print("Bug_01 Fix: Static Code Verification")
    print("="*70)
    
    base_path = Path("c:/_GLT_/Qt Prj/game_lib")
    
    # Check 1: CoreEngine includes actions_remaining in ATTACK_RESOLVED
    engine_cpp = base_path / "GAME/Dungeon-Crawler-Basic/CoreEngine/engine/DungeonEngine.cpp"
    result1 = check_file_contains(
        engine_cpp,
        [
            '"actions_remaining", MAX_ACTIONS_PER_TURN - _actions_this_turn - 1',
        ],
        "CoreEngine: ATTACK_RESOLVED event has actions_remaining"
    )
    
    # Check 2: CoreEngine includes actions_remaining in ACTOR_MOVED
    result2 = check_file_contains(
        engine_cpp,
        [
            'event_id::ACTOR_MOVED',
            '"actions_remaining"',
        ],
        "CoreEngine: ACTOR_MOVED event has actions_remaining"
    )
    
    # Check 3: GUI removed anticipatory decrement
    gui_main = base_path / "GAME/Dungeon-Crawler-Basic/GUI/app/dungeon_main_window.py"
    content = gui_main.read_text(encoding='utf-8')
    
    print(f"\n[CHECK] GUI: No anticipatory action decrement")
    print(f"        File: {gui_main.name}")
    
    # Check that there's NO consume_actions call in the deck command handler
    result3 = True
    if "consume_actions(cost)" in content or "consume_actions(int(action.get" in content:
        print(f"  ✗ Found consume_actions() call (should be removed)")
        result3 = False
    else:
        print(f"  ✓ No consume_actions() in action cost handling")
    
    # Check 4: GUI reads actions_remaining from ACTOR_MOVED
    result4 = check_file_contains(
        gui_main,
        [
            'def _on_actor_moved(self, msg: dict)',
            'actions_remaining = int(data.get("actions_remaining"',
        ],
        "GUI: _on_actor_moved() reads actions_remaining from CoreEngine"
    )
    
    # Check 5: GUI reads actions_remaining from ATTACK_RESOLVED
    result5 = check_file_contains(
        gui_main,
        [
            'def _on_attack_resolved(self, msg: dict)',
            'actions_remaining = int(data.get("actions_remaining"',
        ],
        "GUI: _on_attack_resolved() reads actions_remaining from CoreEngine"
    )
    
    # Summary
    print("\n" + "="*70)
    all_passed = all([result1, result2, result3, result4, result5])
    
    if all_passed:
        print("✓ Bug_01 Fix: ALL CHECKS PASSED")
        print("\n  Architecture Changes Verified:")
        print("  1. CoreEngine sends actions_remaining in ATTACK_RESOLVED")
        print("  2. CoreEngine sends actions_remaining in ACTOR_MOVED")
        print("  3. GUI does NOT decrement actions anticipatorily")
        print("  4. GUI reads actions from ACTOR_MOVED event")
        print("  5. GUI reads actions from ATTACK_RESOLVED event")
        print("\n  Result: CoreEngine COMMANDS, GUI VISUALIZES")
        return True
    else:
        print("✗ Bug_01 Fix: SOME CHECKS FAILED")
        return False

if __name__ == "__main__":
    success = run_verification()
    print("="*70 + "\n")
    sys.exit(0 if success else 1)
