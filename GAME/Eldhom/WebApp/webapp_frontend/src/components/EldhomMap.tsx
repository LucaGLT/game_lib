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
 */
import { useMemo } from 'react'
import type { ActorToken, MapEdge, MapLocation } from '../engine/gameState'

export interface EldhomMapProps {
  locations: MapLocation[]
  edges: MapEdge[]
  tokens: ActorToken[]
  activeActorId: string
  /** Called with a location id when the map is clicked (Phase 4: move targeting). */
  onLocationClick?: (locationId: string) => void
  /** Called with an actor id when a token is clicked (Phase 4: attack targeting). */
  onTokenClick?: (actorId: string) => void
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

const NODE_WIDTH = 120
const NODE_HEIGHT = 56
const LAYER_SPACING = 160
const ROW_SPACING = 84
const MARGIN_X = 70
const MARGIN_Y = 40

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

export function EldhomMap({
  locations,
  edges,
  tokens,
  activeActorId,
  onLocationClick,
  onTokenClick,
}: EldhomMapProps) {
  const layout = useMemo(() => computeLayout(locations, edges), [locations, edges])

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

  return (
    <svg
      className="eldhom-map"
      viewBox={`0 0 ${layout.width} ${layout.height}`}
      role="img"
      aria-label="Mappa della missione"
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
              className={`eldhom-map__node${onLocationClick ? ' eldhom-map__node--clickable' : ''}`}
              onClick={onLocationClick ? () => onLocationClick(node.id) : undefined}
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
  )
}
