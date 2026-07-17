import { useEffect, useRef } from 'react'

interface MatchLogProps {
  entries: string[]
}

/** Append-only match log — mirrors the desktop `LogWidget` (auto-scrolls to the latest line). */
function MatchLog({ entries }: MatchLogProps) {
  const containerRef = useRef<HTMLDivElement | null>(null)

  useEffect(() => {
    const node = containerRef.current
    if (node) {
      node.scrollTop = node.scrollHeight
    }
  }, [entries])

  return (
    <div className="match-log" ref={containerRef} role="log" aria-label="Log della partita">
      {entries.map((line, index) => (
        <div key={index} className="match-log__line">
          {line}
        </div>
      ))}
    </div>
  )
}

export default MatchLog
