import { useEffect, useRef } from 'react'

export interface EventLogProps {
  entries: string[]
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
      {entries.map((line, index) => (
        <div key={index} className="gmgui-event-log__line">
          {line}
        </div>
      ))}
    </div>
  )
}
