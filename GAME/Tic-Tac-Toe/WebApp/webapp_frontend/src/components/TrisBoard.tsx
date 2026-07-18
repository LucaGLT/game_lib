import { LINE_CELLS, cellKey } from '../engine/contract'

interface TrisBoardProps {
  cells: Record<string, string>
  winLine: string | null
  disabled: boolean
  onCellClick: (row: number, col: number) => void
}

const ROWS = [1, 2, 3]
const COLS = [1, 2, 3]

/** The 3x3 clickable Tris grid — visual counterpart of the desktop `BoardWidget`. */
function TrisBoard({ cells, winLine, disabled, onCellClick }: TrisBoardProps) {
  const winningCells = new Set(
    winLine !== null ? (LINE_CELLS[winLine] ?? []).map(([row, col]) => cellKey(row, col)) : [],
  )

  return (
    <div className="tris-board" role="grid" aria-label="Tabellone">
      {ROWS.map((row) =>
        COLS.map((col) => {
          const key = cellKey(row, col)
          const mark = cells[key] ?? ''
          const isWinning = winningCells.has(key)
          return (
            <button
              key={key}
              type="button"
              className={`tris-cell${isWinning ? ' tris-cell--win' : ''}`}
              disabled={disabled || mark !== ''}
              onClick={() => onCellClick(row, col)}
              aria-label={`Cella riga ${row} colonna ${col}`}
            >
              {mark}
            </button>
          )
        }),
      )}
    </div>
  )
}

export default TrisBoard
