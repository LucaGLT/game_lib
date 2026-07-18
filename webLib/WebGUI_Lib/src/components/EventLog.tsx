import { useEffect, useRef } from 'react'

/** One log line: either a plain string (no colour) or a coloured entry (hex/CSS colour string). */
export type EventLogEntry = string | { text: string; color?: string }

export interface EventLogProps {
  entries: EventLogEntry[]
  ariaLabel?: string
}

/** Generic append-only, auto-scrolling event log — web equivalent of the desktop `LogWidget`. */
export function EventLog({ entries, ariaLabel = 'Log degli eventi' }: EventLogProps) {
  const containerRef = useRef<HTMLDivElement | null>(null)

  useEffect(() => {
    const node = containerRef.current
    if (node) {
      node.scrollTop = node.scrollHeight
    }
  }, [entries])

  return (
    <div className="gmgui-event-log" ref={containerRef} role="log" aria-label={ariaLabel}>
      {entries.map((entry, index) => {
        const text = typeof entry === 'string' ? entry : entry.text
        const color = typeof entry === 'string' ? undefined : entry.color
        return (
          <div key={index} className="gmgui-event-log__line" style={color ? { color } : undefined}>
            {text}
          </div>
        )
      })}
    </div>
  )
}
