"""auth_router — ready-made, game-agnostic ``/auth`` FastAPI router.

Any ``eng_serve`` app can mount this unchanged::

    from gmWebServe.auth_router import router as auth_router
    app.include_router(auth_router)

as long as it publishes ``app.state.auth_service`` (a
:class:`gmWebServe.auth.AuthService`) during its lifespan startup — see
:mod:`gmWebServe.fastapi_deps` for the same convention.
"""
from __future__ import annotations

from fastapi import APIRouter, Header, HTTPException, Request
from pydantic import BaseModel

from .auth import InvalidCredentialsError, InvalidTokenError
from .fastapi_deps import extract_bearer_token

__all__ = ["router"]

router = APIRouter(prefix="/auth", tags=["auth"])


class LoginRequest(BaseModel):
    """Body of ``POST /auth/login``."""

    username: str
    password: str


class LoginResponse(BaseModel):
    """Response of ``POST /auth/login``: the bearer token to use on every later call."""

    token: str
    username: str
    expires_at: float


class CurrentUserResponse(BaseModel):
    """Response of ``GET /auth/me``."""

    username: str


@router.post("/login", response_model=LoginResponse)
def login(payload: LoginRequest, request: Request) -> LoginResponse:
    """Verifies credentials and returns a new bearer token.

    Raises:
        HTTPException: 401 if the username/password do not match.
    """
    auth_service = request.app.state.auth_service
    try:
        session = auth_service.login(payload.username, payload.password)
    except InvalidCredentialsError as exc:
        raise HTTPException(status_code=401, detail="Credenziali non valide") from exc
    return LoginResponse(token=session.token, username=session.username, expires_at=session.expires_at)


@router.post("/logout", status_code=204)
def logout(request: Request, authorization: str | None = Header(default=None)) -> None:
    """Revokes the caller's bearer token, if any (idempotent, never fails)."""
    auth_service = request.app.state.auth_service
    token = extract_bearer_token(authorization)
    if token is not None:
        auth_service.logout(token)


@router.get("/me", response_model=CurrentUserResponse)
def me(request: Request, authorization: str | None = Header(default=None)) -> CurrentUserResponse:
    """Returns the username owning the caller's bearer token.

    Used by the frontend to validate a token restored from storage (e.g.
    after a page reload) without needing a dedicated "ping" endpoint.

    Raises:
        HTTPException: 401 if the token is missing, unknown, or expired.
    """
    auth_service = request.app.state.auth_service
    token = extract_bearer_token(authorization)
    if token is None:
        raise HTTPException(status_code=401, detail="Token mancante")
    try:
        username = auth_service.authenticate(token)
    except InvalidTokenError as exc:
        raise HTTPException(status_code=401, detail="Token non valido o scaduto") from exc
    return CurrentUserResponse(username=username)
