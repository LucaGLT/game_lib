/**
 * restClient — thin fetch wrapper for an eng_serve-style REST API (session
 * create + command). Generic across games: the session-creation payload and
 * command typeId/data are caller-supplied, nothing here assumes a specific
 * game's fields (e.g. Tris' `starter_mode`).
 *
 * Requests use relative paths so each app's Vite dev server proxy (see its
 * own vite.config.ts) forwards them to eng_serve without any CORS
 * configuration needed in dev.
 */
import type { SessionInfo } from './types'

/** Boots the engine subprocess and starts a new session (POST /sessions). */
export async function createSession(
  requestBody: Record<string, unknown> = {},
): Promise<SessionInfo> {
  const response = await fetch('/sessions', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(requestBody),
  })
  if (!response.ok) {
    throw new Error(`createSession failed: HTTP ${response.status}`)
  }
  return (await response.json()) as SessionInfo
}

/** Forwards one command envelope (e.g. gmTris.move) to the running engine. */
export async function sendCommand(
  sessionId: string,
  typeId: string,
  data: Record<string, unknown>,
): Promise<void> {
  const response = await fetch(`/sessions/${sessionId}/command`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ type_id: typeId, data }),
  })
  if (!response.ok) {
    throw new Error(`sendCommand failed: HTTP ${response.status}`)
  }
}
