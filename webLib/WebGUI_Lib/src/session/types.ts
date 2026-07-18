/**
 * types — wire-level shapes shared by every eng_serve-style session client
 * (REST + WebSocket). Generic across games: no gmTris/gmMap-specific fields.
 */

export interface EngineEnvelope {
  typeId: string
  source?: string
  data: Record<string, unknown>
  [key: string]: unknown
}

export type EnvelopeHandler = (envelope: EngineEnvelope) => void

export interface SessionInfo {
  session_id: string
  status: string
}
