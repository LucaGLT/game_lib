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
  /** Short human-shareable code a DIFFERENT user types in to join this SAME session. */
  join_code: string
  /** Every seat of this session, role name -> username currently holding it (or null if free). */
  roles: Record<string, string | null>
  /** The role the CALLER holds in this session, or null if they hold none (should not happen). */
  your_role: string | null
}
