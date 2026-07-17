/**
 * wsClient — connects to eng_serve's per-session WebSocket event stream.
 *
 * Streams the exact same typeId/payload envelopes emitted by tris_engine
 * (e.g. `gmMap.snapshot`, `gmMap.cell_changed`) — no translation is applied
 * here; components decide how to interpret each typeId.
 */

export interface EngineEnvelope {
  typeId: string
  source?: string
  data: Record<string, unknown>
  [key: string]: unknown
}

export type EnvelopeHandler = (envelope: EngineEnvelope) => void

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
