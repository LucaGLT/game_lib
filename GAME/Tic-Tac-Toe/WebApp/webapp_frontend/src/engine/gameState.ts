/**
 * gameState — client-side state + reducer for the Tris match, ported from the
 * PySide6 reference handlers in `GAME/Tic-Tac-Toe/GUI/app/tris_window.py`
 * (TurnHeaderWidget / TurnFooterWidget / LogWidget / ErrorBarWidget combined
 * into one plain state object, since Phase 1's scope is a single local user
 * controlling both players — same as the desktop GUI today).
 */
import type { EngineEnvelope } from '@webgui/session/types'
import {
  type ActorSnapshotData,
  type ActorStatusData,
  type CellChangedData,
  type DiceRolledData,
  type GameWonData,
  type InvalidMoveData,
  type MapSnapshotData,
  type PhaseChangedData,
  EVT_ACTOR_SNAPSHOT,
  EVT_CELL_CHANGED,
  EVT_DICE_ROLLED,
  EVT_GAME_DRAW,
  EVT_GAME_WON,
  EVT_INVALID_MOVE,
  EVT_MAP_SNAPSHOT,
  EVT_PHASE_CHANGED,
  EVT_SESSION_STARTED,
  EVT_STATUS_ADDED,
  EVT_STATUS_REMOVED,
  STATUS_ACTIVE_TURN,
  cellKey,
  markOf,
} from './contract'

export type HeaderStatus =
  | { kind: 'waiting' }
  | { kind: 'turn'; mark: string }
  | { kind: 'winner'; mark: string }
  | { kind: 'draw' }

export interface GameState {
  cells: Record<string, string>
  activeMark: string | null
  gameOver: boolean
  header: HeaderStatus
  winLine: string | null
  logEntries: string[]
  errorMessage: string | null
}

export const initialGameState: GameState = {
  cells: {},
  activeMark: null,
  gameOver: false,
  header: { kind: 'waiting' },
  winLine: null,
  logEntries: [],
  errorMessage: null,
}

const MAX_LOG_LINES = 500

function timestamp(): string {
  return new Date().toLocaleTimeString('it-IT', { hour12: false })
}

function withLog(state: GameState, line: string): GameState {
  const entries = [...state.logEntries, `[${timestamp()}] ${line}`]
  return {
    ...state,
    logEntries:
      entries.length > MAX_LOG_LINES ? entries.slice(entries.length - MAX_LOG_LINES) : entries,
  }
}

/** Local, immediate transition for the "Nuova Partita" click (mirrors `_on_reload`). */
export function requestNewGame(state: GameState, starterMode: string): GameState {
  return withLog(
    { ...state, gameOver: false, errorMessage: null },
    `Richiesta nuova partita (modalità: ${starterMode}).`,
  )
}

/** Applies one engine envelope to the game state (mirrors TrisWindow's handler map). */
export function applyEnvelope(state: GameState, envelope: EngineEnvelope): GameState {
  const data = (envelope.data ?? {}) as Record<string, unknown>

  switch (envelope.typeId) {
    case EVT_ACTOR_SNAPSHOT: {
      // Per-actor status history/display is owned by the generic
      // `ActorStatusBadges` module (webgui-lib), subscribed independently on
      // the same router. This reducer only cares who can currently move.
      const { actors } = data as unknown as ActorSnapshotData
      const activeActor = (actors ?? []).find((actor) =>
        (actor.statuses ?? []).includes(STATUS_ACTIVE_TURN),
      )
      if (activeActor) {
        const mark = markOf(activeActor.actor_id)
        return { ...state, activeMark: mark, header: { kind: 'turn', mark } }
      }
      return state
    }

    case EVT_MAP_SNAPSHOT: {
      const { cells } = data as unknown as MapSnapshotData
      const cellMap: Record<string, string> = {}
      for (const cell of cells ?? []) {
        cellMap[cellKey(cell.row, cell.col)] = cell.mark ?? ''
      }
      // Mirrors BoardWidget.reset() being called before re-populating cells:
      // any previous winning-line highlight no longer applies.
      return { ...state, cells: cellMap, winLine: null }
    }

    case EVT_SESSION_STARTED:
      return withLog({ ...state, gameOver: false, errorMessage: null }, 'Partita iniziata.')

    case EVT_PHASE_CHANGED: {
      const { phase } = data as unknown as PhaseChangedData
      return phase === 'GAME_OVER' ? { ...state, gameOver: true } : state
    }

    case EVT_STATUS_ADDED: {
      const { actor_id: actorId, status } = data as unknown as ActorStatusData
      if (status === STATUS_ACTIVE_TURN) {
        const mark = markOf(actorId)
        return { ...state, activeMark: mark, header: { kind: 'turn', mark } }
      }
      return state
    }

    case EVT_STATUS_REMOVED:
      // No Tris-specific reaction needed: turn changes are driven by the
      // ACTIVE_TURN *addition* on the next actor (EVT_STATUS_ADDED above).
      // Per-actor status history now lives only in `ActorStatusBadges`.
      return state

    case EVT_CELL_CHANGED: {
      const { row, col, mark } = data as unknown as CellChangedData
      const next: GameState = {
        ...state,
        cells: { ...state.cells, [cellKey(row, col)]: mark },
        errorMessage: null,
      }
      return withLog(next, `Player ${mark} gioca in (${row}, ${col}).`)
    }

    case EVT_GAME_WON: {
      const { player, line } = data as unknown as GameWonData
      const next: GameState = {
        ...state,
        gameOver: true,
        winLine: line,
        header: { kind: 'winner', mark: player },
      }
      return withLog(next, `Player ${player} vince (${line}).`)
    }

    case EVT_GAME_DRAW:
      return withLog({ ...state, gameOver: true, header: { kind: 'draw' } }, 'Partita pareggiata.')

    case EVT_INVALID_MOVE: {
      const { reason } = data as unknown as InvalidMoveData
      return withLog(
        { ...state, errorMessage: `Mossa non valida: ${reason}.` },
        `Mossa rifiutata (${reason}).`,
      )
    }

    case EVT_DICE_ROLLED: {
      const { value, first } = data as unknown as DiceRolledData
      return withLog(state, `Dado 1d2 = ${value} → inizia Player ${first}.`)
    }

    default:
      return state
  }
}
