/**
 * authClient — thin fetch wrapper for an eng_serve-style pilot-grade auth API
 * (login/logout/me). Generic across games: no game-specific fields, mirrors
 * the shape of `gmWebServe.auth_router` (pyLib/gmWebServe/auth_router.py).
 *
 * Requests use relative paths so each app's Vite dev server proxy forwards
 * them to eng_serve without any CORS configuration needed in dev.
 */

export interface AuthSession {
  token: string
  username: string
  expiresAt: number
}

interface LoginResponseBody {
  token: string
  username: string
  expires_at: number
}

/** Logs in with username/password and returns the issued bearer token (POST /auth/login). */
export async function login(username: string, password: string): Promise<AuthSession> {
  const response = await fetch('/auth/login', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ username, password }),
  })
  if (!response.ok) {
    throw new Error(response.status === 401 ? 'Credenziali non valide.' : `login failed: HTTP ${response.status}`)
  }
  const body = (await response.json()) as LoginResponseBody
  return { token: body.token, username: body.username, expiresAt: body.expires_at }
}

/** Revokes *token* server-side (POST /auth/logout). Best-effort: never throws. */
export async function logout(token: string): Promise<void> {
  try {
    await fetch('/auth/logout', {
      method: 'POST',
      headers: { Authorization: `Bearer ${token}` },
    })
  } catch {
    // The client-side session is always cleared by the caller regardless
    // (see AuthProvider.logout) — a failed revoke here just means the token
    // will sit unused server-side until its own expiry.
  }
}

/**
 * Validates a stored token against the server (GET /auth/me).
 *
 * @returns The username if *token* is still valid, otherwise null.
 */
export async function fetchCurrentUser(token: string): Promise<string | null> {
  const response = await fetch('/auth/me', {
    headers: { Authorization: `Bearer ${token}` },
  })
  if (!response.ok) {
    return null
  }
  const body = (await response.json()) as { username: string }
  return body.username
}
