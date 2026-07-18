/**
 * dragPayload — shared HTML5 drag&drop payload format for dragging a card.
 * Used by `DeckTable` (drag source: Mano/Mazzo/Scarti, drop targets:
 * Giocate/Memoria/Mano) AND `EldhomMap` (drop targets: location nodes/actor
 * tokens, for playing a hand card that needs a destination/target) — a
 * single shared MIME type/shape keeps both components speaking the same
 * "wire format" for the payload regardless of which one reads it.
 */
export const DRAG_MIME = 'application/x-eldhom-card'

export interface DragPayload {
  cardId: string
  from: 'CardHand' | 'MainDeck' | 'DiscardPile'
}

export function setDragPayload(event: React.DragEvent, payload: DragPayload): void {
  event.dataTransfer.setData(DRAG_MIME, JSON.stringify(payload))
  event.dataTransfer.effectAllowed = 'move'
}

export function readDragPayload(event: React.DragEvent): DragPayload | null {
  const raw = event.dataTransfer.getData(DRAG_MIME)
  if (!raw) {
    return null
  }
  try {
    return JSON.parse(raw) as DragPayload
  } catch {
    return null
  }
}
