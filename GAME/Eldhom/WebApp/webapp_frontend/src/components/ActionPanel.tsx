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
  /** The currently active actor's hero name (properly capitalized) — shown in the panel title as "Azione di {name}", regardless of who is viewing (the turn-status message addressing the VIEWING participant lives in App.tsx instead, near the role banner). */
  activeHeroName: string
  enabled: boolean
  sequenceActive: boolean
  /** True if the active hero has at least one legal/meaningful action (a melee target, an interactable object, something to heal, or a playable card) besides Fine Turno — see App.tsx. When false, the 4 base actions are disabled (Fine Turno never is). */
  hasAnyAction: boolean
  /** True if the active hero already completed their one allowed action/card/sequence this turn and must press Fine Turno to confirm before the turn actually passes — see `EldhomState.turnAwaitingConfirmation`. Like `hasAnyAction`, this disables the 4 base actions but never Fine Turno; unlike it, it also takes priority for the hint message shown. */
  awaitingConfirmation: boolean
  targetingMode: TargetingMode
  pendingReaction: PendingReactionView | null
  onArmMove: () => void
  onArmAttack: () => void
  onInteract: () => void
  onRecover: () => void
  onEndTurn: () => void
  onStopSequence: () => void
  onReactionChosen: (reaction: string) => void
}

const REACTION_LABELS: Record<string, string> = {
  TAKE: '☠ Subisci',
  BLOCK: '⛨ Para',
  DODGE: '↺ Schiva',
}

export function ActionPanel({
  activeHeroName,
  enabled,
  sequenceActive,
  hasAnyAction,
  awaitingConfirmation,
  targetingMode,
  pendingReaction,
  onArmMove,
  onArmAttack,
  onInteract,
  onRecover,
  onEndTurn,
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

  // Explicit user request: the player must ALWAYS be able to confirm Fine
  // Turno, even (especially) when nothing else is worth doing OR when they
  // already acted this turn and are only awaiting confirmation — so ONLY the
  // 4 base actions are gated by `hasAnyAction`/`awaitingConfirmation`, never
  // the end-turn button.
  const baseActionsDisabled = !enabled || sequenceActive || !hasAnyAction || awaitingConfirmation
  return (
    <div className="eldhom-actions">
      <span className="eldhom-actions__title">Azione di {activeHeroName !== '' ? activeHeroName : '—'}</span>
      <div className="eldhom-actions__buttons">
        <button type="button" disabled={baseActionsDisabled} onClick={onArmMove}>
          {targetingMode === 'move' ? '✕ Annulla Muovi' : '▶️ 2◻️ : 2⏳'}
        </button>
        <button type="button" disabled={baseActionsDisabled} onClick={onArmAttack}>
          {targetingMode === 'attack' ? '✕ Annulla Attacco' : '⏸️⚔️ 1❌ : 2⏳'}
        </button>
        <button type="button" disabled={baseActionsDisabled} onClick={onInteract}>
          ⏺️ : 3⏳
        </button>
        <button type="button" disabled={baseActionsDisabled} onClick={onRecover}>
          +1❤️ ♻️1🂠 : 3⏳
        </button>
        {!sequenceActive && (
          <button
            type="button"
            className="eldhom-actions__end-turn"
            disabled={!enabled}
            onClick={onEndTurn}
            title="Conferma esplicitamente la fine del tuo turno, anche se potresti agire ancora"
          >
            🏁 Fine Turno
          </button>
        )}
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
      {enabled && !sequenceActive && awaitingConfirmation && (
        <p className="eldhom-actions__hint">
          ✅ Hai già agito questo turno — premi 🏁 Fine Turno per confermare e passare al prossimo Attore
        </p>
      )}
      {enabled && !sequenceActive && !awaitingConfirmation && !hasAnyAction && (
        <p className="eldhom-actions__hint">⚠ Nessuna azione disponibile — premi 🏁 Fine Turno per passare</p>
      )}
    </div>
  )
}
