"""Dungeon Crawler Basic — event router.

EventRouter dispatches incoming engine event envelopes to the registered
widget handlers by ``typeId``. It provides a clean decoupling between the
bridge layer (which receives raw dicts) and the individual widgets (which only
care about specific event types).

Usage::

    router = EventRouter()
    router.register("dungeon.actor.moved",  actor_panel.on_envelope)
    router.register("dungeon.map.snapshot", board_widget.on_envelope)
    router.dispatch(msg)  # called for every incoming event
"""
from __future__ import annotations

from typing import Callable


class EventRouter:
    """Routes engine event envelopes to per-typeId handler callables.

    Handlers are registered with :meth:`register`. A single event may have
    multiple handlers (all are called in registration order). Unknown typeIds
    are silently ignored.
    """

    def __init__(self) -> None:
        """Initialises an empty routing table."""
        # ToBeImplemented //
        self._handlers: dict[str, list[Callable[[dict], None]]] = {}

    def register(self, type_id: str, handler: Callable[[dict], None]) -> None:
        """Registers a handler for a specific event typeId.

        Args:
            type_id: Event typeId string (e.g. ``"dungeon.actor.moved"``).
            handler: Callable receiving the full event dict.
        """
        # ToBeImplemented //
        self._handlers.setdefault(type_id, []).append(handler)

    def dispatch(self, msg: dict) -> None:
        """Dispatches an event dict to all registered handlers for its typeId.

        Args:
            msg: Decoded event dict with at least a ``typeId`` key.
        """
        # ToBeImplemented //
        type_id = msg.get("typeId", "")
        for handler in self._handlers.get(type_id, []):
            handler(msg)
