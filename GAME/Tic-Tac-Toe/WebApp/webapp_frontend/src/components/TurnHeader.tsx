import type { HeaderStatus } from '../engine/gameState'

interface TurnHeaderProps {
  status: HeaderStatus
}

function textFor(status: HeaderStatus): string {
  switch (status.kind) {
    case 'waiting':
      return "In attesa dell'inizio della partita…"
    case 'turn':
      return `Turno di:  Player ${status.mark}  (${status.mark})`
    case 'winner':
      return `🏆  Ha vinto Player ${status.mark}!`
    case 'draw':
      return 'Partita pareggiata.'
  }
}

/** Central turn/result banner — mirrors the desktop `TurnHeaderWidget`. */
function TurnHeader({ status }: TurnHeaderProps) {
  return <h2 className={`turn-header turn-header--${status.kind}`}>{textFor(status)}</h2>
}

export default TurnHeader
