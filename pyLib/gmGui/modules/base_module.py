"""IGmGuiModule — public module contract; BaseModule — common boilerplate."""
from __future__ import annotations

from abc import ABC, abstractmethod
from typing import TYPE_CHECKING

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QWidget

if TYPE_CHECKING:
    from ..engine_bridge.sender import EngineSender


class IGmGuiModule(ABC):
    """Public contract for all GameLib GUI modules.

    Each concrete module maps 1:1 to one C++ library and is housed in a
    ``QDockWidget``.  ``MainWindow`` interacts with modules exclusively through
    this interface — no concrete module type ever leaks into ``MainWindow``.

    Mandatory overrides
    -------------------
    - :attr:`module_id`
    - :attr:`title`
    - :attr:`default_area`
    - :meth:`widget` (provided by :class:`BaseModule` via caching)
    - :meth:`subscribed_type_ids`
    - :meth:`on_envelope`

    Optional overrides (have sensible defaults)
    -------------------------------------------
    - :meth:`on_attach`
    - :meth:`on_detach`
    - :meth:`save_state`
    - :meth:`restore_state`
    """

    # ── Identity / layout ─────────────────────────────────────────────────────

    @property
    @abstractmethod
    def module_id(self) -> str:
        """Unique string key; used as ``QDockWidget.objectName`` for QSettings."""

    @property
    @abstractmethod
    def title(self) -> str:
        """Human-readable title shown in the QDockWidget title bar."""

    @property
    @abstractmethod
    def default_area(self) -> Qt.DockWidgetArea:
        """Dock area used on first launch (before any persisted layout is loaded)."""

    @abstractmethod
    def widget(self) -> QWidget:
        """Returns the root widget placed inside the QDockWidget.

        Implementations must return the **same** instance on every call.
        :class:`BaseModule` satisfies this contract via a cache.
        """

    # ── Engine bridge ─────────────────────────────────────────────────────────

    @abstractmethod
    def subscribed_type_ids(self) -> list[str]:
        """List of ``gmDispatch`` typeId strings this module wants to receive.

        ``MainWindow`` builds its routing table from these lists at startup.
        Use ``"*"`` to receive every typeId (not recommended for performance).
        """

    @abstractmethod
    def on_envelope(self, msg: dict) -> None:
        """Processes one incoming envelope from the engine bridge.

        Always called on the **Qt main thread** (routed via a cross-thread Signal).

        Args:
            msg: Deserialised envelope dict with keys:
                 ``"typeId"`` (str), ``"source"`` (str),
                 ``"data"`` (dict), ``"time"`` (str ISO-8601).
        """

    # ── Lifecycle (default implementations — override as needed) ──────────────

    def on_attach(self) -> None:
        """Called by MainWindow after the QDockWidget has been added to the layout.

        Override to start timers, subscribe to additional signals, etc.
        """

    def on_detach(self) -> None:
        """Called by MainWindow just before application shutdown.

        Override to stop timers and release resources.
        """

    def save_state(self) -> dict:
        """Returns a JSON-serialisable dict of module-specific state for QSettings.

        The default implementation returns an empty dict (no state to persist).
        """
        return {}

    def restore_state(self, state: dict) -> None:
        """Restores state previously returned by :meth:`save_state`.

        The default implementation is a no-op.
        """

    def send_command(self, type_id: str, data: dict) -> None:
        """Sends a command envelope to the C++ engine via EngineSender.

        No-op if the sender has not been injected yet (safe to call at any time).

        Args:
            type_id: ``gmDispatch`` typeId string, e.g. ``"gmFlow.session.pause"``.
            data:    JSON-serialisable dict carrying the command payload.
        """
        if hasattr(self, "_sender") and self._sender is not None:
            self._sender.send_command(type_id, data)


class BaseModule(IGmGuiModule, ABC):
    """Partial implementation of IGmGuiModule with sender injection and widget caching.

    Concrete modules should inherit from :class:`BaseModule`, not from
    :class:`IGmGuiModule` directly, and implement :meth:`_build_widget` and
    :meth:`on_envelope`.
    """

    def __init__(self) -> None:
        self._sender: EngineSender | None = None
        self._widget: QWidget | None = None

    def set_sender(self, sender: EngineSender) -> None:
        """Injects the EngineSender.

        Called by ``MainWindow._register_modules()`` before :meth:`on_attach`.
        """
        self._sender = sender

    def widget(self) -> QWidget:
        """Returns the cached root widget, building it on the first call."""
        if self._widget is None:
            self._widget = self._build_widget()
        return self._widget

    @abstractmethod
    def _build_widget(self) -> QWidget:
        """Constructs and returns the module's root widget.

        Called exactly once by :meth:`widget`.  Subclasses must not call this
        method directly.
        """
