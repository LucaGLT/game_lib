/**
 * logFormat — Eldhôm-specific event → human-readable narrative text. Port
 * of GAME/Eldhom/GUI/widgets/log_widget.py's `_format_event`/`on_any_event`/
 * `on_action_result`/`on_mission_victory`/`on_mission_defeat`/`on_next_actor`.
 *
 * Icons/text AND per-event-type colours are ported (colours restored after
 * an initial Phase 5 pass had dropped them — see PLAN.md's visual-overhaul
 * notes): each line carries the exact hex colour `log_widget.py` uses for
 * that event type, rendered by `EventLog` (`@webgui/*`, extended to accept
 * `{text, color}` entries alongside plain strings for backward
 * compatibility with Tris' plain-string usage).
 *
 * `lastDisplayedTime` is deliberately private module state (not part of the
 * `gameState.ts` reducer): it is pure log-presentation bookkeeping (which
 * "Tempo N" gaps have already been printed), not game domain state, mirroring
 * the desktop `LogWidget`'s own instance field `_last_displayed_time`. Call
 * `resetLogTimeTracking()` when a new mission starts.
 */
import type { EventLogEntry } from '@webgui/components/EventLog'
import type { EngineEnvelope } from '@webgui/session/types'
import type { NextActorWire } from './contract'

const EVENT_TEMPLATES: Record<string, { template: string; color: string }> = {
  'eldhom.pg.played_card': { template: '{actor} gioca {payload}', color: '#d4b07a' },
  'eldhom.pg.simple_action': { template: '{actor} esegue azione semplice', color: '#c8c870' },
  'eldhom.pg.moved': { template: '{actor} si sposta → {payload}', color: '#80c0e0' },
  'eldhom.pg.attacked': { template: '{actor} attacca {payload}', color: '#e08080' },
  'eldhom.pg.healed': { template: '{actor} si cura (+{payload} PV)', color: '#80e080' },
  'eldhom.pg.ko': { template: '{actor} è KO!', color: '#ff6060' },
  'eldhom.pg.turn_ended': { template: '{actor} termina il turno', color: '#888888' },
  'eldhom.monster.damaged': { template: 'Mostro {actor} danneggiato', color: '#e07070' },
  'eldhom.monster.defeated': { template: 'Mostro {actor} eliminato!', color: '#ff9900' },
  'eldhom.monster.moved': { template: '{actor} si sposta → {payload}', color: '#e0c080' },
  'eldhom.group.activated': { template: 'Gruppo {actor} — turno completato', color: '#c07070' },
  'eldhom.group.eliminated': { template: 'Gruppo {actor} ELIMINATO!', color: '#ff6600' },
  'eldhom.formation.changed': { template: 'Formazione cambiata: {payload}', color: '#a070d0' },
  'eldhom.deck.reshuffled': { template: '{actor}: mazzo rimescolato', color: '#7090a0' },
  'eldhom.mission.time_advanced': { template: '⌛ {actor} → Tempo {payload}', color: '#607080' },
}

let lastDisplayedTime = -1

/** Resets the "Tempo N" gap-filling tracker — call when a new mission starts. */
export function resetLogTimeTracking(): void {
  lastDisplayedTime = -1
}

/** Formats one engine envelope into zero or more coloured narrative log lines (mirrors `on_any_event`). */
export function formatEvent(envelope: EngineEnvelope): EventLogEntry[] {
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
    const text =
      attackType === 'RANGED' && range > 0
        ? `${attacker} attacca ${target}: 🏹 ${range}▸ ${damage}❌`
        : `${attacker} attacca ${target}: ⚔ ${damage}❌`
    return [{ text, color: '#e08080' }]
  }

  // Zone-boundary door opened: payload is a dict with the two location ids.
  if (typeId === 'eldhom.zone_door.opened' && payload !== null && typeof payload === 'object') {
    const p = payload as { a?: string; b?: string }
    return [{ text: `🚪 ${actorId || '?'} apre il passaggio ${p.a ?? '?'} ↔ ${p.b ?? '?'}`, color: '#e0a050' }]
  }

  if (typeId === 'eldhom.turn.next_actor') {
    return formatNextActor(data as unknown as NextActorWire)
  }

  if (typeId === 'eldhom.action.result') {
    return data.ok === false
      ? [{ text: `⚠ ${String(data.error ?? 'Errore sconosciuto')}`, color: '#e08060' }]
      : []
  }

  if (typeId === 'eldhom.mission.victory') {
    return [{ text: '🏆 MISSIONE COMPLETATA — Vittoria!', color: '#60e060' }]
  }

  if (typeId === 'eldhom.mission.defeat') {
    return [{ text: `💀 MISSIONE FALLITA — ${String(payload ?? '')}`, color: '#e06060' }]
  }

  const entry = EVENT_TEMPLATES[typeId]
  if (entry === undefined) {
    return []
  }
  const payloadStr = typeof payload === 'string' ? payload : payload === undefined ? '' : String(payload)
  const text = entry.template.replace('{actor}', actorId || '?').replace('{payload}', payloadStr)
  return [{ text, color: entry.color }]
}

/** Logs timeline progression: empty slots then the acting actor's turn (mirrors `on_next_actor`). */
function formatNextActor(data: NextActorWire): EventLogEntry[] {
  const lines: EventLogEntry[] = []
  const actorTimeline = data.actor_timeline ?? data.mission_time ?? 0
  const actorName = data.actor_name || data.actor_id
  const kind = data.kind || 'HERO'
  const emptySlotColor = '#506070'

  const MAX_EMPTY = 8
  const previous = lastDisplayedTime
  if (previous >= 0 && actorTimeline > previous + 1) {
    const emptyCount = actorTimeline - previous - 1
    const showCount = Math.min(emptyCount, MAX_EMPTY)
    for (let t = previous + 1; t < previous + 1 + showCount; t += 1) {
      lines.push({ text: `⌛ Tempo ${t}: Nessun Attore`, color: emptySlotColor })
    }
    if (emptyCount > MAX_EMPTY) {
      lines.push({ text: `  … [${emptyCount - MAX_EMPTY} slot vuoti omessi]`, color: emptySlotColor })
    }
  }

  lastDisplayedTime = actorTimeline
  const color = kind === 'HERO' ? '#a8c8e8' : '#e0a888'
  const label = kind === 'HERO' ? `▶ Gioca ${actorName}` : `★ Attiva ${actorName}`
  lines.push({ text: `⌛ Tempo ${actorTimeline}: ${label}`, color })
  return lines
}
