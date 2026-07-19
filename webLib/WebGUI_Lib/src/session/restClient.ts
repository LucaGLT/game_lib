/**
 * restClient — thin fetch wrapper for an eng_serve-style REST API (session
 * list/create/close + command). Generic across games: the session-creation
 * payload and command typeId/data are caller-supplied, nothing here assumes
 * a specific game's fields (e.g. Tris' `starter_mode`).
 *
 * Requests use relative paths so each app's Vite dev server proxy (see its
 * own vite.config.ts) forwards them to eng_serve without any CORS
 * configuration needed in dev. Every call requires a bearer *token* from
 * `session/authClient.ts` (Phase 2: pilot-grade multi-user auth) — the
 * server isolates each user's sessions from every other user's.
 */
import type { SessionInfo } from './types'

function authHeaders(token: string): Record<string, string> {
  return { Authorization: `Bearer ${token}` }
}

/** Boots the engine subprocess and starts a new session (POST /sessions). */
export async function createSession(
  token: string,
  requestBody: Record<string, unknown> = {},
): Promise<SessionInfo> {
  const response = await fetch('/sessions', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', ...authHeaders(token) },
    body: JSON.stringify(requestBody),
  })
  if (!response.ok) {
    throw new Error(`createSession failed: HTTP ${response.status}`)
  }
  return (await response.json()) as SessionInfo
}

/** Lists every session the caller participates in, created or joined (GET /sessions). */
export async function listSessions(token: string): Promise<SessionInfo[]> {
  const response = await fetch('/sessions', { headers: authHeaders(token) })
  if (!response.ok) {
    throw new Error(`listSessions failed: HTTP ${response.status}`)
  }
  return (await response.json()) as SessionInfo[]
}

/** Returns one session's current status, e.g. to poll for a shared match's seats (GET /sessions/{id}). */
export async function getSession(token: string, sessionId: string): Promise<SessionInfo> {
  const response = await fetch(`/sessions/${sessionId}`, { headers: authHeaders(token) })
  if (!response.ok) {
    throw new Error(`getSession failed: HTTP ${response.status}`)
  }
  return (await response.json()) as SessionInfo
}

/**
 * Joins the SAME session/match as its creator, using the `join_code` they
 * shared out-of-band (e.g. verbally) — this is what makes a match truly
 * multiplayer (two different users piloting two different seats of ONE
 * shared session), as opposed to each user getting their own isolated match.
 */
export async function joinSession(token: string, joinCode: string): Promise<SessionInfo> {
  const response = await fetch('/sessions/join', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', ...authHeaders(token) },
    body: JSON.stringify({ join_code: joinCode }),
  })
  if (!response.ok) {
    throw new Error(`joinSession failed: HTTP ${response.status}`)
  }
  return (await response.json()) as SessionInfo
}

/** Forwards one command envelope (e.g. gmTris.move) to the running engine. */
export async function sendCommand(
  token: string,
  sessionId: string,
  typeId: string,
  data: Record<string, unknown>,
): Promise<void> {
  const response = await fetch(`/sessions/${sessionId}/command`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', ...authHeaders(token) },
    body: JSON.stringify({ type_id: typeId, data }),
  })
  if (!response.ok) {
    throw new Error(`sendCommand failed: HTTP ${response.status}`)
  }
}

/** Closes one session, freeing its slot in the per-user concurrent-session cap. */
export async function closeSession(token: string, sessionId: string): Promise<void> {
  const response = await fetch(`/sessions/${sessionId}`, {
    method: 'DELETE',
    headers: authHeaders(token),
  })
  if (!response.ok && response.status !== 404) {
    throw new Error(`closeSession failed: HTTP ${response.status}`)
  }
}
