/**
 * ActorDetailModal — "extended card" pop-up shown when the player clicks a
 * synthetic card (`HeroPanel`/`MonsterGroupPanel`) in `App.tsx`'s hero row.
 * Explicit user request (Phase 20): *"quando Clicco su una Card Sintetica
 * si deve aprire in Pop-Up una pagina che mostra la CARDE ESTESA con tutte
 * le info del PG o Mostro — Equipaggiamento, Statistiche, altro ancora da
 * definire — vedi come era il pyQt quando selezioni un Actor"*.
 *
 * Section layout mirrors the desktop's generic actor detail panel
 * (`pyLib/gmGui/modules/gm_actor_module.py`'s `GmActorModule`: header
 * name/faction/state chips, HP bar, Status list, Risorse list, Equipaggiamento
 * list) as closely as the current `eldhom.*` wire contract allows. Reading
 * that module (and its Eldhôm-specific translator,
 * `GAME/Eldhom/GUI/widgets/eldhom_actor_adapter.py`) shows Equipaggiamento/
 * Status are ALREADY always-empty placeholders for Eldhôm today too — the
 * C++ CoreEngine's `eldhom.state.full` JSON (`GAME/Eldhom/CoreEngine/main.cpp`)
 * never serializes an `equipment`/`statuses` field for heroes or monster
 * instances, so the desktop's adapter always defaults them to `{}`. This
 * modal is upfront about that (explicit "Non ancora disponibile" placeholder)
 * rather than fabricating data the engine doesn't send — matching the
 * project's convention for known-incomplete features (e.g. the "🚫 Bandite"
 * button, `MainMenuModal`'s 3 disabled entries).
 *
 * What IS real and shown: HP (aggregate, across all instances for a
 * monster group), position/location, timeline, faction/monster_type, and
 * — for heroes only, real wire data not otherwise surfaced by the compact
 * `HeroPanel` — deck/discard/hand/played counts. For monster groups, a
 * per-instance breakdown table (HP, alive/defeated, location), using the
 * same map-consistent labels as `EldhomMap`/`ActorToken` (looked up via the
 * `tokens` prop) instead of recomputing the label heuristic independently.
 */
import { Modal } from '@webgui/components/Modal'
import type { HeroWire, MonsterGroupWire } from '../engine/contract'
import type { ActorToken } from '../engine/gameState'

export type ActorDetailSubject =
  | { kind: 'hero'; hero: HeroWire }
  | { kind: 'monsterGroup'; group: MonsterGroupWire }

export interface ActorDetailModalProps {
  subject: ActorDetailSubject
  /** Used only to resolve map-consistent instance labels (e.g. "B1") for a monster group's breakdown table. */
  tokens: ActorToken[]
  onDismiss: () => void
}

const LIFE_STATE_LABELS: Record<number, string> = {
  0: 'Attivo',
  1: 'KO',
  2: 'Morto',
}

/** "brigante_comune" -> "Brigante Comune". */
function formatMonsterType(monsterType: string): string {
  return monsterType
    .split('_')
    .filter((word) => word.length > 0)
    .map((word) => word[0].toUpperCase() + word.slice(1))
    .join(' ')
}

export function ActorDetailModal({ subject, tokens, onDismiss }: ActorDetailModalProps) {
  const isHero = subject.kind === 'hero'
  const name = isHero ? subject.hero.name : subject.group.name
  const factionLabel = isHero ? subject.hero.faction : formatMonsterType(subject.group.monster_type)

  const hp = isHero
    ? Math.max(0, subject.hero.hp)
    : subject.group.instances.reduce((sum, instance) => sum + Math.max(0, instance.hp), 0)
  const maxHp = isHero
    ? subject.hero.max_hp
    : subject.group.instances.reduce((sum, instance) => sum + instance.max_hp, 0)
  const hpRatio = maxHp > 0 ? hp / maxHp : 0

  const aliveCount = isHero ? null : subject.group.instances.filter((instance) => instance.alive).length
  const totalCount = isHero ? null : subject.group.instances.length
  const stateLabel = isHero
    ? (LIFE_STATE_LABELS[subject.hero.life_state] ?? 'Attivo')
    : `${aliveCount}/${totalCount} vivi`

  const tokensByActorId = new Map(tokens.map((token) => [token.actorId, token]))

  return (
    <Modal title={name} onDismiss={onDismiss}>
      <div className="eldhom-actor-detail">
        <div className="eldhom-actor-detail__chips">
          <span className="eldhom-actor-detail__chip">{factionLabel}</span>
          <span
            className={`eldhom-actor-detail__chip${isHero ? '' : ' eldhom-actor-detail__chip--enemy'}`}
          >
            {stateLabel}
          </span>
        </div>

        <div className="eldhom-hero-panel__hp-track eldhom-actor-detail__hp-track">
          <div className="eldhom-hero-panel__hp-fill" style={{ width: `${hpRatio * 100}%` }} />
          <span className="eldhom-hero-panel__hp-text">
            ❤ {hp}/{maxHp}
          </span>
        </div>

        <section className="eldhom-actor-detail__section">
          <p className="eldhom-actor-detail__section-title">Risorse</p>
          <p>⌛ Tempo: {isHero ? subject.hero.timeline : subject.group.timeline}</p>
          {isHero && (
            <p>
              📍 {subject.hero.location} (
              {subject.hero.position === 'FRONTLINE' ? 'Primo piano' : 'Retro'})
            </p>
          )}
        </section>

        {isHero && (
          <section className="eldhom-actor-detail__section">
            <p className="eldhom-actor-detail__section-title">Mazzo</p>
            <p>🂠 Mazzo: {subject.hero.deck_count}</p>
            <p>🗑 Scarti: {subject.hero.discard_count}</p>
            <p>
              🖐 Mano: {subject.hero.hand.length}/{subject.hero.hand_limit}
            </p>
            <p>🂡 Giocate: {subject.hero.played_ids.length}</p>
          </section>
        )}

        {!isHero && (
          <section className="eldhom-actor-detail__section">
            <p className="eldhom-actor-detail__section-title">Istanze</p>
            <ul className="eldhom-actor-detail__instance-list">
              {subject.group.instances.map((instance) => (
                <li
                  key={instance.id}
                  className={`eldhom-actor-detail__instance${
                    instance.alive ? '' : ' eldhom-actor-detail__instance--dead'
                  }`}
                >
                  <span className="eldhom-actor-detail__instance-id">
                    {tokensByActorId.get(instance.id)?.label ?? instance.id}
                  </span>
                  <span>
                    ❤ {Math.max(0, instance.hp)}/{instance.max_hp}
                  </span>
                  <span>📍 {instance.location}</span>
                  <span>{instance.alive ? 'Vivo' : 'Sconfitto'}</span>
                </li>
              ))}
            </ul>
          </section>
        )}

        <section className="eldhom-actor-detail__section">
          <p className="eldhom-actor-detail__section-title">Equipaggiamento</p>
          <p className="eldhom-actor-detail__placeholder">Non ancora disponibile</p>
        </section>

        <section className="eldhom-actor-detail__section">
          <p className="eldhom-actor-detail__section-title">Stato / Effetti attivi</p>
          <p className="eldhom-actor-detail__placeholder">Non ancora disponibile</p>
        </section>
      </div>
    </Modal>
  )
}
