/**
 * restClient — thin fetch wrapper for the eng_serve REST API (Phase 1: no auth).
 *
 * Requests use relative paths so the Vite dev server proxy (see vite.config.ts)
 * forwards them to eng_serve without any CORS configuration needed in dev.
 */

export interface SessionInfo {
  session_id: string
  status: string
}

/** Boots the engine subprocess and starts a new match (POST /sessions). */
export async function createSession(starterMode: string = 'fixed_x'): Promise<SessionInfo> {
  const response = await fetch('/sessions', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ starter_mode: starterMode }),
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
