"""gmGui.message_ids — shared, game-agnostic typeId constants.

These typeId strings define the reusable contract between any GameLib GUI and
any CoreEngine. They are intentionally **not** game-specific so every game that
reuses :class:`gmGui.main_window.MainWindow` and the shared modules speaks the
same protocol.

Area-info contract
------------------
- :data:`AREA_INFO_REQUEST` (GUI → Core): the GUI asks the engine for the
  contents of an area. Payload: ``{ "area_id": str, "request_id"?: str }``.
- :data:`AREA_INFO_RESPONSE` (Core → GUI): the engine returns the area
  contents. Payload::

      {
        "area_id": str,
        "actors": [ { "id": str, "name": str, "faction"?: str, "state"?: str }, ... ],
        "interactables": [ { "id": str, "name": str, "type"?: str, "state"?: str }, ... ],
        "request_id"?: str
      }

- :data:`AREA_SELECTED` (GUI-internal, optional): emitted by the map module when
  the user selects an area. Payload: ``{ "area_id": str }``.

Design principle
----------------
A user interaction on the map must only change visualisations (its own widget
or other widgets) and request data — it must **never** trigger actor actions.
"""
from __future__ import annotations

# ── Area-info contract (reusable across all games) ────────────────────────────
AREA_INFO_REQUEST: str = "gmMap.area.info.request"
AREA_INFO_RESPONSE: str = "gmMap.area.info.response"
AREA_SELECTED: str = "gmMap.ui.area_selected"
