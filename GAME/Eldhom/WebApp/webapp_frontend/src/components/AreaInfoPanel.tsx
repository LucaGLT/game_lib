/**
 * AreaInfoPanel — details of the map location currently selected (name,
 * adjacent locations, actors present). Port of
 * GAME/Eldhom/GUI/widgets/area_info_widget.py's `AreaInfoWidget`.
 *
 * Purely presentational: all data is supplied by the parent from the
 * cached game state (same design as the desktop widget, which performs no
 * networking itself either).
 */
export interface AreaInfoActor {
  actorId: string
  label: string
  isHero: boolean
  hp: number
  maxHp: number
  position: 'FRONTLINE' | 'BACKLINE'
}

export interface AreaInfoPanelProps {
  locationName: string | null
  adjacentNames: string[]
  actors: AreaInfoActor[]
  onActorClick?: (actorId: string) => void
}

export function AreaInfoPanel({
  locationName,
  adjacentNames,
  actors,
  onActorClick,
}: AreaInfoPanelProps) {
  return (
    <div className="eldhom-area-info">
      <p className="eldhom-area-info__title">
        {locationName === null ? 'Nessuna area selezionata' : `Area: ${locationName}`}
      </p>
      {locationName !== null && (
        <>
          <p className="eldhom-area-info__adjacent">
            {adjacentNames.length > 0
              ? `Adiacenti: ${adjacentNames.join(', ')}`
              : 'Adiacenti: (nessuna)'}
          </p>
          <p className="eldhom-area-info__actors-title">Attori presenti</p>
          <ul className="eldhom-area-info__actors">
            {actors.length === 0 && <li className="eldhom-area-info__empty">(area vuota)</li>}
            {actors.map((actor) => (
              <li key={actor.actorId}>
                <button
                  type="button"
                  className="eldhom-area-info__actor"
                  onClick={onActorClick ? () => onActorClick(actor.actorId) : undefined}
                >
                  {actor.isHero ? '♦' : '☠'} {actor.label} ❤ {actor.hp}/{actor.maxHp} [
                  {actor.position}]
                </button>
              </li>
            ))}
          </ul>
        </>
      )}
    </div>
  )
}
