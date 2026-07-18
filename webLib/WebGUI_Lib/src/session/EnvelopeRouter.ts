/**
 * EnvelopeRouter — routes incoming engine envelopes to whichever modules
 * subscribed to their typeId. Web equivalent of the desktop `MainWindow`'s
 * routing table (`dict[typeId -> [IGmGuiModule]]` in
 * `pyLib/gmGui/main_window.py`): it lets multiple independent modules share
 * ONE WebSocket connection without knowing about each other.
 */
import type { EngineEnvelope, EnvelopeHandler } from './types'

const WILDCARD = '*'

export class EnvelopeRouter {
  private readonly handlers = new Map<string, Set<EnvelopeHandler>>()
  private readonly wildcardHandlers = new Set<EnvelopeHandler>()

  /**
   * Registers `handler` for every typeId in `typeIds`. Use `"*"` to receive
   * every envelope (not recommended for performance — mirrors the desktop
   * `IGmGuiModule.subscribed_type_ids` docstring). Returns an unsubscribe
   * function.
   */
  subscribe(typeIds: readonly string[], handler: EnvelopeHandler): () => void {
    if (typeIds.includes(WILDCARD)) {
      this.wildcardHandlers.add(handler)
      return () => {
        this.wildcardHandlers.delete(handler)
      }
    }

    for (const typeId of typeIds) {
      const existing = this.handlers.get(typeId) ?? new Set<EnvelopeHandler>()
      existing.add(handler)
      this.handlers.set(typeId, existing)
    }

    return () => {
      for (const typeId of typeIds) {
        this.handlers.get(typeId)?.delete(handler)
      }
    }
  }

  /** Dispatches one envelope to every handler subscribed to its typeId (plus wildcard handlers). */
  dispatch(envelope: EngineEnvelope): void {
    this.handlers.get(envelope.typeId)?.forEach((handler) => handler(envelope))
    this.wildcardHandlers.forEach((handler) => handler(envelope))
  }
}
