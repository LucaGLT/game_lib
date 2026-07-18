/**
 * wsClient — connects to an eng_serve-style per-session WebSocket event
 * stream. Streams whatever typeId/payload envelopes the backend replays —
 * no translation is applied here; callers (typically an `EnvelopeRouter`)
 * decide how to interpret each typeId.
 */
import type { EngineEnvelope, EnvelopeHandler } from './types'

/**
 * Opens a WebSocket to `/sessions/{sessionId}/ws` and forwards every parsed
 * envelope to `onEnvelope`. Returns a `disconnect` function that closes the
 * socket.
 */
export function connectSessionEvents(sessionId: string, onEnvelope: EnvelopeHandler): () => void {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  const socket = new WebSocket(`${protocol}//${window.location.host}/sessions/${sessionId}/ws`)

  socket.onmessage = (event: MessageEvent<string>) => {
    const envelope = JSON.parse(event.data) as EngineEnvelope
    onEnvelope(envelope)
  }

  return () => socket.close()
}
