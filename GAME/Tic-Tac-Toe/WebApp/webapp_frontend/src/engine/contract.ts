/**
 * contract — the native `gmTris` wire contract (typeId + payload shapes),
 * ported from the PySide6 reference (`GAME/Tic-Tac-Toe/GUI/app/tris_window.py`
 * and `modules/gm_tris_board_module.py`) so the WebApp parses the exact same
 * engine events instead of just dumping raw JSON.
 */

export const EVT_ACTOR_SNAPSHOT = 'gmActor.snapshot'
export const EVT_MAP_SNAPSHOT = 'gmMap.snapshot'
export const EVT_SESSION_STARTED = 'gmFlow.session.started'
export const EVT_PHASE_CHANGED = 'gmFlow.session.phase_changed'
export const EVT_STATUS_ADDED = 'gmActor.actor.status_added'
export const EVT_STATUS_REMOVED = 'gmActor.actor.status_removed'
export const EVT_CELL_CHANGED = 'gmMap.cell_changed'
export const EVT_GAME_WON = 'gmRules.game_won'
export const EVT_GAME_DRAW = 'gmRules.game_draw'
export const EVT_INVALID_MOVE = 'gmTris.invalid_move'
export const EVT_DICE_ROLLED = 'gmAlea.dice_rolled'

export const CMD_MOVE = 'gmTris.move'
export const CMD_NEW_GAME = 'gmTris.new_game'

export const STATUS_ACTIVE_TURN = 'ACTIVE_TURN'

export interface ActorSummary {
  actor_id: string
  display_name?: string
  statuses: string[]
}

export interface ActorSnapshotData {
  actors: ActorSummary[]
}

export interface CellData {
  row: number
  col: number
  mark: string
}

export interface MapSnapshotData {
  cells: CellData[]
}

export interface CellChangedData {
  row: number
  col: number
  mark: string
}

export interface PhaseChangedData {
  phase: string
}

export interface ActorStatusData {
  actor_id: string
  status: string
}

export interface GameWonData {
  player: string
  line: string
}

export interface InvalidMoveData {
  reason: string
}

export interface DiceRolledData {
  value: number | string
  first: string
}

/** Maps a WinRules line id (from `gmRules.game_won`) to its 3 board cells (1-based). */
export const LINE_CELLS: Record<string, Array<[number, number]>> = {
  row_1: [
    [1, 1],
    [1, 2],
    [1, 3],
  ],
  row_2: [
    [2, 1],
    [2, 2],
    [2, 3],
  ],
  row_3: [
    [3, 1],
    [3, 2],
    [3, 3],
  ],
  col_1: [
    [1, 1],
    [2, 1],
    [3, 1],
  ],
  col_2: [
    [1, 2],
    [2, 2],
    [3, 2],
  ],
  col_3: [
    [1, 3],
    [2, 3],
    [3, 3],
  ],
  diag_main: [
    [1, 1],
    [2, 2],
    [3, 3],
  ],
  diag_anti: [
    [1, 3],
    [2, 2],
    [3, 1],
  ],
}

/** Returns `"X"` or `"O"` from an actor id such as `"Player_X"`. */
export function markOf(actorId: string): string {
  return actorId.endsWith('X') ? 'X' : 'O'
}

export function cellKey(row: number, col: number): string {
  return `${row}-${col}`
}
