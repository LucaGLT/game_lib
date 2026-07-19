"""fastapi_deps — reusable FastAPI auth dependencies for any eng_serve gateway.

Every consuming app must set ``app.state.auth_service`` to a
:class:`gmWebServe.auth.AuthService` instance during its lifespan startup
(see ``eng_serve/main.py`` in each game's WebApp); these dependencies read it
from there, so they stay 100% game-agnostic.
"""
from __future__ import annotations

from fastapi import Header, HTTPException, Request

from .auth import InvalidTokenError

__all__ = ["extract_bearer_token", "get_current_user", "authenticate_ws"]


def extract_bearer_token(authorization: str | None) -> str | None:
    """Extracts the token from an ``Authorization: Bearer <token>`` header.

    Returns:
        The token, or None if *authorization* is missing or not a Bearer header.
    """
    if authorization is None or not authorization.startswith("Bearer "):
        return None
    return authorization[len("Bearer "):]


def get_current_user(request: Request, authorization: str | None = Header(default=None)) -> str:
    """FastAPI dependency: resolves the caller's Bearer token to a username.

    Usage::

        @router.get("/sessions")
        def list_sessions(request: Request, owner: str = Depends(get_current_user)):
            ...

    Raises:
        HTTPException: 401 if the header is missing or the token is unknown/expired.
    """
    token = extract_bearer_token(authorization)
    if token is None:
        raise HTTPException(status_code=401, detail="Autenticazione richiesta")
    try:
        return request.app.state.auth_service.authenticate(token)
    except InvalidTokenError as exc:
        raise HTTPException(status_code=401, detail="Token non valido o scaduto") from exc


def authenticate_ws(app_state, token: str | None) -> str | None:
    """Resolves a WebSocket query-param token to a username, or None if invalid.

    WebSocket handshakes cannot set custom headers from browser JavaScript,
    so the token travels as a ``?token=`` query parameter instead of a
    header — callers should close the connection (e.g. code 4401) when this
    returns None, without accepting the socket first.
    """
    if token is None:
        return None
    try:
        return app_state.auth_service.authenticate(token)
    except InvalidTokenError:
        return None
