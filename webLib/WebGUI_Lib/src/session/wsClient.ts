/**
 * wsClient — connects to an eng_serve-style per-session WebSocket event
 * stream. Streams whatever typeId/payload envelopes the backend replays —
 * no translation is applied here; callers (typically an `EnvelopeRouter`)
 * decide how to interpret each typeId.
 *
 * Requires a bearer *token* from `session/authClient.ts` (Phase 2): WebSocket
 * handshakes cannot set custom headers from browser JavaScript, so the token
 * travels as a `?token=` query parameter instead.
 */
import type { EngineEnvelope, EnvelopeHandler } from './types'

/**
 * Opens a WebSocket to `/sessions/{sessionId}/ws?token=...` and forwards
 * every parsed envelope to `onEnvelope`. Returns a `disconnect` function
 * that closes the socket.
 */
export function connectSessionEvents(
  token: string,
  sessionId: string,
  onEnvelope: EnvelopeHandler,
): () => void {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  const url = `${protocol}//${window.location.host}/sessions/${sessionId}/ws?token=${encodeURIComponent(token)}`
  const socket = new WebSocket(url)

  socket.onmessage = (event: MessageEvent<string>) => {
    const envelope = JSON.parse(event.data) as EngineEnvelope
    onEnvelope(envelope)
  }

  return () => socket.close()
}
