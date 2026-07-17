/**
 * EldhomMap — locations + adjacencies rendered as a graph, with actor
 * tokens placed at their current location. Port of
 * `GAME/Eldhom/GUI/widgets/board_widget.py`'s `EldhomBoardWidget` (which
 * wraps the generic `gmGui.modules.gm_map_module.GmMapModule` /
 * `gmGui/widgets/map_scene.py`) to React/SVG.
 *
 * Layout: the desktop widget uses `GmMapModule`'s force-directed physics
 * layout; this port uses a simpler deterministic BFS-layered layout
 * (locations placed left-to-right by BFS distance from the mission's first
 * location) — visually different but functionally equivalent for Phase 3's
 * goal (correct locations/adjacencies/tokens/edge styles), without pulling
 * in a physics/graph-layout dependency.
 *
 * Edge colours are intentionally NOT theme-reactive: `map_scene.py`'s
 * `_edge_pen()` documents these as "fixed gameplay-state semantics, not
 * theme-dependent" (`pyLib/gmGui/theme_manager.py::resolve_semantic_color`,
 * tokens `map_edge_locked_door` / `map_actor_turn_border`) — CLOSED_DOOR and
 * LOCKED_DOOR share the same red (#c03030), differing only by dash style;
 * the active-turn token border is a separate fixed red (#e03030). Only
 * FREE edges and the map panel/border chrome follow the active theme
 * (`--gm-*` vars), same as `border`'s semantic token being theme-dependent.
 *
 * Zoom/pan: mouse-wheel zoom (centred on the cursor) and click-drag pan are
 * implemented by adjusting an SVG `viewBox` rectangle over the fixed-size
 * "world" produced by `computeLayout` — the layout itself never changes,
 * only which portion of it is visible. The wheel listener is attached
 * natively (not via React's `onWheel`) because React 17+ registers
 * `onWheel` as a passive listener, which would silently ignore
 * `preventDefault()` and let the whole page scroll instead of zooming the
 * map.
 */
import { useEffect, useMemo, useRef, useState } from 'react'
import type { ActorToken, MapEdge, MapLocation } from '../engine/gameState'
import { readDragPayload } from '../engine/dragPayload'

export interface EldhomMapProps {
  locations: MapLocation[]
  edges: MapEdge[]
  tokens: ActorToken[]
  activeActorId: string
  /** Called with a location id when the map is clicked (Phase 4: move targeting). */
  onLocationClick?: (locationId: string) => void
  /** Called with an actor id when a token is clicked (Phase 4: attack targeting). */
  onTokenClick?: (actorId: string) => void
  /** Called with a hand card id and a location id when that card is dropped on this location (MOVE-effect cards). */
  onCardDropOnLocation?: (cardId: string, locationId: string) => void
  /** Called with a hand card id and an actor id when that card is dropped on this token (DAMAGE-effect cards). */
  onCardDropOnToken?: (cardId: string, actorId: string) => void
}

interface LayoutNode {
  id: string
  name: string
  x: number
  y: number
}

interface MapLayout {
  nodes: LayoutNode[]
  nodeById: Map<string, LayoutNode>
  width: number
  height: number
}

interface ViewBox {
  x: number
  y: number
  w: number
  h: number
}

const NODE_WIDTH = 130
const NODE_HEIGHT = 62
const LAYER_SPACING = 190
const ROW_SPACING = 100
const MARGIN_X = 90
const MARGIN_Y = 60
const MIN_ZOOM = 0.4
const MAX_ZOOM = 3.5
const ZOOM_STEP = 1.15

/** BFS-layers every location (by distance from the mission's first location), one pass per connected component. */
function computeLayout(locations: MapLocation[], edges: MapEdge[]): MapLayout {
  const adjacency = new Map<string, Set<string>>()
  const neighborsOf = (id: string): Set<string> => {
    let set = adjacency.get(id)
    if (!set) {
      set = new Set()
      adjacency.set(id, set)
    }
    return set
  }
  for (const location of locations) {
    neighborsOf(location.id)
  }
  for (const edge of edges) {
    neighborsOf(edge.a).add(edge.b)
    neighborsOf(edge.b).add(edge.a)
  }

  const layerRows: string[][] = []
  const remaining = new Set(locations.map((location) => location.id))
  let layerOffset = 0

  while (remaining.size > 0) {
    const root = locations.find((location) => remaining.has(location.id))?.id
    if (root === undefined) {
      break
    }
    let frontier = [root]
    let layer = layerOffset
    while (frontier.length > 0) {
      const rowIds: string[] = []
      const next: string[] = []
      for (const id of frontier) {
        if (!remaining.has(id)) {
          continue
        }
        remaining.delete(id)
        rowIds.push(id)
        for (const neighbor of neighborsOf(id)) {
          if (remaining.has(neighbor)) {
            next.push(neighbor)
          }
        }
      }
      if (rowIds.length > 0) {
        layerRows[layer] = [...(layerRows[layer] ?? []), ...rowIds]
        layer += 1
      }
      frontier = next
    }
    layerOffset = Math.max(layerOffset, layer)
  }

  const nodes: LayoutNode[] = []
  let maxRows = 1
  layerRows.forEach((ids, layer) => {
    maxRows = Math.max(maxRows, ids.length)
    ids.forEach((id, row) => {
      const location = locations.find((candidate) => candidate.id === id)
      nodes.push({
        id,
        name: location?.name ?? id,
        x: MARGIN_X + layer * LAYER_SPACING,
        y: MARGIN_Y + row * ROW_SPACING,
      })
    })
  })

  const width = MARGIN_X * 2 + Math.max(layerRows.length - 1, 0) * LAYER_SPACING + NODE_WIDTH
  const height = MARGIN_Y * 2 + Math.max(maxRows - 1, 0) * ROW_SPACING + NODE_HEIGHT
  return { nodes, nodeById: new Map(nodes.map((node) => [node.id, node])), width, height }
}

function fitViewBox(layout: MapLayout): ViewBox {
  return { x: 0, y: 0, w: layout.width, h: layout.height }
}

export function EldhomMap({
  locations,
  edges,
  tokens,
  activeActorId,
  onLocationClick,
  onTokenClick,
  onCardDropOnLocation,
  onCardDropOnToken,
}: EldhomMapProps) {
  const layout = useMemo(() => computeLayout(locations, edges), [locations, edges])
  const svgRef = useRef<SVGSVGElement | null>(null)
  const [viewBox, setViewBox] = useState<ViewBox>(() => fitViewBox(layout))
  const [dragOverId, setDragOverId] = useState<string | null>(null)
  const panState = useRef<{ pointerId: number; startClientX: number; startClientY: number; startViewBox: ViewBox } | null>(null)

  // Resets the view whenever the underlying map changes (new mission, or a
  // rebuild triggered by a newly-unlocked passage) so zoom/pan never leaves
  // the player stuck looking at an empty area of a now-different map.
  useEffect(() => {
    setViewBox(fitViewBox(layout))
  }, [layout])

  // Native (non-passive) wheel listener — see module docstring for why this
  // cannot be a React onWheel handler.
  useEffect(() => {
    const svg = svgRef.current
    if (!svg) {
      return undefined
    }
    function handleWheel(event: WheelEvent): void {
      event.preventDefault()
      const svgEl = svgRef.current
      if (!svgEl) {
        return
      }
      const rect = svgEl.getBoundingClientRect()
      setViewBox((previous) => {
        const zoomFactor = event.deltaY < 0 ? 1 / ZOOM_STEP : ZOOM_STEP
        const currentZoom = layout.width / previous.w
        const nextZoom = Math.min(MAX_ZOOM, Math.max(MIN_ZOOM, currentZoom / zoomFactor))
        const newW = layout.width / nextZoom
        const newH = layout.height / nextZoom
        // Cursor position in SVG user-space, kept fixed under the pointer while zooming.
        const cursorX = previous.x + ((event.clientX - rect.left) / rect.width) * previous.w
        const cursorY = previous.y + ((event.clientY - rect.top) / rect.height) * previous.h
        const newX = cursorX - ((cursorX - previous.x) / previous.w) * newW
        const newY = cursorY - ((cursorY - previous.y) / previous.h) * newH
        return { x: newX, y: newY, w: newW, h: newH }
      })
    }
    svg.addEventListener('wheel', handleWheel, { passive: false })
    return () => svg.removeEventListener('wheel', handleWheel)
  }, [layout])

  function handlePointerDown(event: React.PointerEvent<SVGSVGElement>): void {
    // Only pan when the drag starts on the empty map background, not on a
    // node/token (those have their own click handlers and must not also
    // trigger a pan).
    if (event.target !== event.currentTarget) {
      return
    }
    panState.current = {
      pointerId: event.pointerId,
      startClientX: event.clientX,
      startClientY: event.clientY,
      startViewBox: viewBox,
    }
    event.currentTarget.setPointerCapture(event.pointerId)
  }

  function handlePointerMove(event: React.PointerEvent<SVGSVGElement>): void {
    const pan = panState.current
    if (!pan || pan.pointerId !== event.pointerId) {
      return
    }
    const rect = event.currentTarget.getBoundingClientRect()
    const dxSvg = ((event.clientX - pan.startClientX) / rect.width) * pan.startViewBox.w
    const dySvg = ((event.clientY - pan.startClientY) / rect.height) * pan.startViewBox.h
    setViewBox({ ...pan.startViewBox, x: pan.startViewBox.x - dxSvg, y: pan.startViewBox.y - dySvg })
  }

  function handlePointerUp(event: React.PointerEvent<SVGSVGElement>): void {
    if (panState.current?.pointerId === event.pointerId) {
      panState.current = null
    }
  }

  function zoomBy(factor: number): void {
    setViewBox((previous) => {
      const currentZoom = layout.width / previous.w
      const nextZoom = Math.min(MAX_ZOOM, Math.max(MIN_ZOOM, currentZoom * factor))
      const newW = layout.width / nextZoom
      const newH = layout.height / nextZoom
      const cx = previous.x + previous.w / 2
      const cy = previous.y + previous.h / 2
      return { x: cx - newW / 2, y: cy - newH / 2, w: newW, h: newH }
    })
  }

  const tokensByLocation = useMemo(() => {
    const map = new Map<string, ActorToken[]>()
    for (const token of tokens) {
      const list = map.get(token.location) ?? []
      list.push(token)
      map.set(token.location, list)
    }
    return map
  }, [tokens])

  if (layout.nodes.length === 0) {
    return <p className="eldhom-map__empty">Nessuna missione in corso: nessuna mappa da mostrare.</p>
  }

  // Adapts the map box's proportions to the actual layout instead of a fixed
  // height — a simple single-row mission (very wide, short layout) no longer
  // wastes a huge blank area below the content, while a tall/branching
  // layout still gets a generously tall box (clamped so neither extreme
  // becomes unusable).
  const mapAspectRatio = Math.min(3, Math.max(0.9, layout.width / layout.height))

  return (
    <div className="eldhom-map-container">
      <div className="eldhom-map__toolbar">
        <button type="button" onClick={() => zoomBy(ZOOM_STEP)} title="Zoom avanti" aria-label="Zoom avanti">
          ＋
        </button>
        <button type="button" onClick={() => zoomBy(1 / ZOOM_STEP)} title="Zoom indietro" aria-label="Zoom indietro">
          －
        </button>
        <button
          type="button"
          onClick={() => setViewBox(fitViewBox(layout))}
          title="Adatta alla vista"
          aria-label="Adatta alla vista"
        >
          ⤢
        </button>
      </div>
      <svg
        ref={svgRef}
        className="eldhom-map"
        style={{ aspectRatio: mapAspectRatio }}
        viewBox={`${viewBox.x} ${viewBox.y} ${viewBox.w} ${viewBox.h}`}
        preserveAspectRatio="xMidYMin meet"
        role="img"
        aria-label="Mappa della missione (rotellina per zoom, trascina per spostare la visuale)"
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onPointerLeave={handlePointerUp}
      >

      {edges.map((edge) => {
        const from = layout.nodeById.get(edge.a)
        const to = layout.nodeById.get(edge.b)
        if (!from || !to) {
          return null
        }
        return (
          <line
            key={`${edge.a}|${edge.b}`}
            x1={from.x + NODE_WIDTH / 2}
            y1={from.y + NODE_HEIGHT / 2}
            x2={to.x + NODE_WIDTH / 2}
            y2={to.y + NODE_HEIGHT / 2}
            className={`eldhom-map__edge eldhom-map__edge--${edge.type.toLowerCase()}`}
          />
        )
      })}
      {layout.nodes.map((node) => {
        const locationTokens = tokensByLocation.get(node.id) ?? []
        return (
          <foreignObject key={node.id} x={node.x} y={node.y} width={NODE_WIDTH} height={NODE_HEIGHT}>
            <div
              className={[
                'eldhom-map__node',
                onLocationClick ? 'eldhom-map__node--clickable' : '',
                onCardDropOnLocation && dragOverId === node.id ? 'eldhom-map__node--drag-over' : '',
              ]
                .filter(Boolean)
                .join(' ')}
              onClick={onLocationClick ? () => onLocationClick(node.id) : undefined}
              onDragOver={
                onCardDropOnLocation
                  ? (event) => {
                      event.preventDefault()
                      setDragOverId(node.id)
                    }
                  : undefined
              }
              onDragLeave={onCardDropOnLocation ? () => setDragOverId(null) : undefined}
              onDrop={
                onCardDropOnLocation
                  ? (event) => {
                      event.preventDefault()
                      setDragOverId(null)
                      const payload = readDragPayload(event)
                      if (payload && payload.from === 'CardHand') {
                        onCardDropOnLocation(payload.cardId, node.id)
                      }
                    }
                  : undefined
              }
            >
              <div className="eldhom-map__node-name">{node.name}</div>
              <div className="eldhom-map__node-tokens">
                {locationTokens.map((token) => (
                  <span
                    key={token.actorId}
                    className={[
                      'eldhom-map__token',
                      onTokenClick ? 'eldhom-map__token--clickable' : '',
                      token.alive ? '' : 'eldhom-map__token--dead',
                      token.actorId === activeActorId || token.groupId === activeActorId
                        ? 'eldhom-map__token--active'
                        : '',
                      onCardDropOnToken && dragOverId === token.actorId ? 'eldhom-map__token--drag-over' : '',
                    ]
                      .filter(Boolean)
                      .join(' ')}
                    title={`${token.label} — ${token.hp}/${token.maxHp} PV`}
                    onClick={
                      onTokenClick
                        ? (event) => {
                            event.stopPropagation()
                            onTokenClick(token.actorId)
                          }
                        : undefined
                    }
                    onDragOver={
                      onCardDropOnToken
                        ? (event) => {
                            event.preventDefault()
                            event.stopPropagation()
                            setDragOverId(token.actorId)
                          }
                        : undefined
                    }
                    onDragLeave={
                      onCardDropOnToken
                        ? (event) => {
                            event.stopPropagation()
                            setDragOverId(null)
                          }
                        : undefined
                    }
                    onDrop={
                      onCardDropOnToken
                        ? (event) => {
                            event.preventDefault()
                            event.stopPropagation()
                            setDragOverId(null)
                            const payload = readDragPayload(event)
                            if (payload && payload.from === 'CardHand') {
                              onCardDropOnToken(payload.cardId, token.actorId)
                            }
                          }
                        : undefined
                    }
                  >
                    {token.label}
                  </span>
                ))}
              </div>
            </div>
          </foreignObject>
        )
      })}
      </svg>
    </div>
  )
}
