/**
 * ActionPanel — the 4 Azioni Semplici (Movimento/Attacco/Interazione/
 * Recupero) plus the inline TAKE/BLOCK/DODGE reaction window. Port of
 * GAME/Eldhom/GUI/widgets/action_panel_widget.py's ActionPanelWidget.
 * Costs are the fixed ⌛ values from
 * GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp (COST_SIMPLE_*), duplicated
 * here since eng_serve stays a pure pass-through and does not expose engine
 * constants over the wire.
 *
 * Move/attack targeting (point-and-click via the map) is armed/disarmed
 * here and resolved by the parent (App.tsx), which owns the shared
 * targeting-mode state consumed by both this panel and EldhomMap — same
 * split as the desktop's `move_armed`/`attack_armed` signals resolved by
 * `EldhomMainWindow`.
 *
 * The reaction window is NOT a modal dialog on the desktop either — it is
 * already handled inline inside `ActionPanelWidget` (`enter_defense_mode`),
 * so this component does the same (the 3 real dialogs are Phase 6's scope).
 */
export type TargetingMode = 'move' | 'attack' | null

export interface PendingReactionView {
  defenderName: string
  incomingDamage: number
  reactions: string[]
}

export interface ActionPanelProps {
  heroName: string
  enabled: boolean
  sequenceActive: boolean
  targetingMode: TargetingMode
  pendingReaction: PendingReactionView | null
  onArmMove: () => void
  onArmAttack: () => void
  onInteract: () => void
  onRecover: () => void
  onStopSequence: () => void
  onReactionChosen: (reaction: string) => void
}

const REACTION_LABELS: Record<string, string> = {
  TAKE: '☠ Subisci',
  BLOCK: '⛨ Para',
  DODGE: '↺ Schiva',
}

export function ActionPanel({
  heroName,
  enabled,
  sequenceActive,
  targetingMode,
  pendingReaction,
  onArmMove,
  onArmAttack,
  onInteract,
  onRecover,
  onStopSequence,
  onReactionChosen,
}: ActionPanelProps) {
  if (pendingReaction !== null) {
    return (
      <div className="eldhom-actions eldhom-actions--defense">
        <span className="eldhom-actions__title">
          DIFESA: {pendingReaction.defenderName} — danno in arrivo {pendingReaction.incomingDamage}❌
        </span>
        <div className="eldhom-actions__buttons">
          {pendingReaction.reactions.map((reaction) => (
            <button key={reaction} type="button" onClick={() => onReactionChosen(reaction)}>
              {REACTION_LABELS[reaction] ?? reaction}
            </button>
          ))}
        </div>
      </div>
    )
  }

  return (
    <div className="eldhom-actions">
      <span className="eldhom-actions__title">{enabled ? `TURNO: ${heroName}` : 'In attesa…'}</span>
      <div className="eldhom-actions__buttons">
        <button type="button" disabled={!enabled || sequenceActive} onClick={onArmMove}>
          {targetingMode === 'move' ? '✕ Annulla Muovi' : '▶️ 2◻️ : 2⏳'}
        </button>
        <button type="button" disabled={!enabled || sequenceActive} onClick={onArmAttack}>
          {targetingMode === 'attack' ? '✕ Annulla Attacco' : '⏸️⚔️ 1❌ : 2⏳'}
        </button>
        <button type="button" disabled={!enabled || sequenceActive} onClick={onInteract}>
          ⏺️ : 3⏳
        </button>
        <button type="button" disabled={!enabled || sequenceActive} onClick={onRecover}>
          +1❤️ ♻️1🂠 : 3⏳
        </button>
        {sequenceActive && (
          <button type="button" className="eldhom-actions__stop" onClick={onStopSequence}>
            ■ Stop seq.
          </button>
        )}
      </div>
      {targetingMode === 'move' && (
        <p className="eldhom-actions__hint">▶ Clicca la locazione di destinazione sulla mappa</p>
      )}
      {targetingMode === 'attack' && (
        <p className="eldhom-actions__hint">⚔ Clicca il nemico da attaccare sulla mappa</p>
      )}
    </div>
  )
}
