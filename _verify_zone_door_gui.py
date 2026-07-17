import sys
import os

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, "pyLib")
sys.path.insert(0, "GAME/Eldhom/GUI")

from PySide6.QtWidgets import QApplication
from widgets.board_widget import EldhomBoardWidget

app = QApplication.instance() or QApplication(sys.argv)

base_state = {
    "heroes": [{"id": "thael", "name": "Thael", "hp": 5, "max_hp": 5, "location": "corridoio",
                "position": "FRONTLINE", "faction": "HEROES", "life_state": 0, "hand": [],
                "deck_count": 1, "discard_count": 0, "discard_ids": [], "played_ids": [],
                "hand_limit": 5}],
    "groups": [],
    "locations": [
        {"adjacent": ["sala"], "id": "corridoio", "name": "Corridoio"},
        {"adjacent": ["corridoio"], "id": "sala", "name": "Sala"},
    ],
    "mission_id": "test_zone_door_gui",
    "next_actor": {"actor_id": "thael", "kind": "HERO"},
    "special_objects": [],
}

board = EldhomBoardWidget()

board.on_state_full({"typeId": "eldhom.state.full", "headers": {"data": {**base_state, "opened_zone_doors": []}}})
scene = board._module._map_scene
edge_types_before = {t for _, t in scene._edges}
print("Edge types BEFORE door opened:", edge_types_before)
assert edge_types_before == {"CLOSED_DOOR"}, "Expected the single edge to be CLOSED_DOOR"

board.on_state_full({
    "typeId": "eldhom.state.full",
    "headers": {"data": {**base_state, "opened_zone_doors": [["corridoio", "sala"]]}},
})
edge_types_after = {t for _, t in scene._edges}
print("Edge types AFTER door opened:", edge_types_after)
assert edge_types_after == {"FREE"}, "Expected the single edge to become FREE after opening"

print("GUI zone-door colouring test: OK")
