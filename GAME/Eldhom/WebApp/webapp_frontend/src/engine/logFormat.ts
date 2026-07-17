/**
 * logFormat — Eldhôm-specific event → human-readable narrative text. Port
 * of GAME/Eldhom/GUI/widgets/log_widget.py's `_format_event`/`on_any_event`/
 * `on_action_result`/`on_mission_victory`/`on_mission_defeat`/`on_next_actor`.
 *
 * Icons/text only — colours are dropped: `EventLog` (`@webgui/*`) stays the
 * already-generic plain-string component, no new rendering component is
 * added here, only this formatting function (see
 * GAME/Eldhom/WebApp/PLAN.md, Phase 5).
 *
 * `lastDisplayedTime` is deliberately private module state (not part of the
 * `gameState.ts` reducer): it is pure log-presentation bookkeeping (which
 * "Tempo N" gaps have already been printed), not game domain state, mirroring
 * the desktop `LogWidget`'s own instance field `_last_displayed_time`. Call
 * `resetLogTimeTracking()` when a new mission starts.
 */
import type { EngineEnvelope } from '@webgui/session/types'
import type { NextActorWire } from './contract'

const EVENT_TEMPLATES: Record<string, string> = {
  'eldhom.pg.played_card': '{actor} gioca {payload}',
  'eldhom.pg.simple_action': '{actor} esegue azione semplice',
  'eldhom.pg.moved': '{actor} si sposta → {payload}',
  'eldhom.pg.attacked': '{actor} attacca {payload}',
  'eldhom.pg.healed': '{actor} si cura (+{payload} PV)',
  'eldhom.pg.ko': '{actor} è KO!',
  'eldhom.pg.turn_ended': '{actor} termina il turno',
  'eldhom.monster.damaged': 'Mostro {actor} danneggiato',
  'eldhom.monster.defeated': 'Mostro {actor} eliminato!',
  'eldhom.monster.moved': '{actor} si sposta → {payload}',
  'eldhom.group.activated': 'Gruppo {actor} — turno completato',
  'eldhom.group.eliminated': 'Gruppo {actor} ELIMINATO!',
  'eldhom.formation.changed': 'Formazione cambiata: {payload}',
  'eldhom.deck.reshuffled': '{actor}: mazzo rimescolato',
  'eldhom.mission.time_advanced': '⌛ {actor} → Tempo {payload}',
}

let lastDisplayedTime = -1

/** Resets the "Tempo N" gap-filling tracker — call when a new mission starts. */
export function resetLogTimeTracking(): void {
  lastDisplayedTime = -1
}

/** Formats one engine envelope into zero or more narrative log lines (mirrors `on_any_event`). */
export function formatEvent(envelope: EngineEnvelope): string[] {
  const typeId = envelope.typeId
  const data = envelope.data
  const actorId = String(data.actor_id ?? '')
  const payload = data.payload

  // Enriched attack event: payload is a dict with target/damage/type.
  if (typeId === 'eldhom.pg.attacked' && payload !== null && typeof payload === 'object') {
    const p = payload as Record<string, unknown>
    const target = String(p.target ?? '?')
    const damage = Number(p.damage ?? 0)
    const attackType = String(p.attack_type ?? p.type ?? 'MELEE')
    const range = Number(p.range ?? 0)
    const attacker = actorId || '?'
    if (attackType === 'RANGED' && range > 0) {
      return [`${attacker} attacca ${target}: 🏹 ${range}▸ ${damage}❌`]
    }
    return [`${attacker} attacca ${target}: ⚔ ${damage}❌`]
  }

  // Zone-boundary door opened: payload is a dict with the two location ids.
  if (typeId === 'eldhom.zone_door.opened' && payload !== null && typeof payload === 'object') {
    const p = payload as { a?: string; b?: string }
    return [`🚪 ${actorId || '?'} apre il passaggio ${p.a ?? '?'} ↔ ${p.b ?? '?'}`]
  }

  if (typeId === 'eldhom.turn.next_actor') {
    return formatNextActor(data as unknown as NextActorWire)
  }

  if (typeId === 'eldhom.action.result') {
    return data.ok === false ? [`⚠ ${String(data.error ?? 'Errore sconosciuto')}`] : []
  }

  if (typeId === 'eldhom.mission.victory') {
    return ['🏆 MISSIONE COMPLETATA — Vittoria!']
  }

  if (typeId === 'eldhom.mission.defeat') {
    return [`💀 MISSIONE FALLITA — ${String(payload ?? '')}`]
  }

  const template = EVENT_TEMPLATES[typeId]
  if (template === undefined) {
    return []
  }
  const payloadStr = typeof payload === 'string' ? payload : payload === undefined ? '' : String(payload)
  return [template.replace('{actor}', actorId || '?').replace('{payload}', payloadStr)]
}

/** Logs timeline progression: empty slots then the acting actor's turn (mirrors `on_next_actor`). */
function formatNextActor(data: NextActorWire): string[] {
  const lines: string[] = []
  const actorTimeline = data.actor_timeline ?? data.mission_time ?? 0
  const actorName = data.actor_name || data.actor_id
  const kind = data.kind || 'HERO'

  const MAX_EMPTY = 8
  const previous = lastDisplayedTime
  if (previous >= 0 && actorTimeline > previous + 1) {
    const emptyCount = actorTimeline - previous - 1
    const showCount = Math.min(emptyCount, MAX_EMPTY)
    for (let t = previous + 1; t < previous + 1 + showCount; t += 1) {
      lines.push(`⌛ Tempo ${t}: Nessun Attore`)
    }
    if (emptyCount > MAX_EMPTY) {
      lines.push(`  … [${emptyCount - MAX_EMPTY} slot vuoti omessi]`)
    }
  }

  lastDisplayedTime = actorTimeline
  const label = kind === 'HERO' ? `▶ Gioca ${actorName}` : `★ Attiva ${actorName}`
  lines.push(`⌛ Tempo ${actorTimeline}: ${label}`)
  return lines
}
