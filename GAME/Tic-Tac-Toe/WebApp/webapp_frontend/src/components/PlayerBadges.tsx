const STATUS_LABEL: Record<string, string> = {
  WINNER: 'Vincitore',
  DRAW: 'Pareggio',
  ACTIVE_TURN: 'Turno corrente',
}
const STATUS_PRIORITY = ['WINNER', 'DRAW', 'ACTIVE_TURN']

interface PlayerBadgesProps {
  playerStatuses: Record<string, string[]>
}

function badgeFor(statuses: string[]): { text: string; variant: string } {
  for (const status of STATUS_PRIORITY) {
    if (statuses.includes(status)) {
      return { text: STATUS_LABEL[status], variant: status.toLowerCase() }
    }
  }
  return { text: 'in attesa', variant: 'idle' }
}

/** Two status badges (Player X / Player O) — mirrors the desktop `TurnFooterWidget`. */
function PlayerBadges({ playerStatuses }: PlayerBadgesProps) {
  return (
    <div className="player-badges">
      {['X', 'O'].map((mark) => {
        const actorId = `Player_${mark}`
        const badge = badgeFor(playerStatuses[actorId] ?? [])
        return (
          <span key={mark} className={`player-badge player-badge--${badge.variant}`}>
            Player {mark}: {badge.text}
          </span>
        )
      })}
    </div>
  )
}

export default PlayerBadges
