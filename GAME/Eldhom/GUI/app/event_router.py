"""Le Pergamene di Eldhom — event router.

EventRouter dispatches incoming engine event envelopes to registered
widget handlers by ``typeId``.
"""
from __future__ import annotations

from typing import Callable


class EventRouter:
    """Routes engine event envelopes to per-typeId handler callables."""

    def __init__(self) -> None:
        """Initialises an empty routing table."""
        self._handlers: dict[str, list[Callable[[dict], None]]] = {}

    def register(self, type_id: str, handler: Callable[[dict], None]) -> None:
        """Registers a handler for a specific event typeId.

        Args:
            type_id: Event typeId string (e.g. ``"eldhom.turn.next_actor"``).
            handler: Callable receiving the full event dict.
        """
        self._handlers.setdefault(type_id, []).append(handler)

    def dispatch(self, msg: dict) -> None:
        """Dispatches an event dict to all registered handlers.

        Args:
            msg: Decoded event dict with at least a ``typeId`` key.
        """
        type_id = msg.get("typeId", "")
        for handler in self._handlers.get(type_id, []):
            try:
                handler(msg)
            except Exception as exc:  # noqa: BLE001
                print(f"[EventRouter] handler error for {type_id}: {exc}")
