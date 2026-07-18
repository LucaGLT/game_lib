export interface ErrorBarProps {
  message: string | null
}

/** Generic bottom message bar (idle vs error state) — web equivalent of the desktop `ErrorBarWidget`. */
export function ErrorBar({ message }: ErrorBarProps) {
  const isError = message !== null
  return (
    <p className={`gmgui-error-bar${isError ? ' gmgui-error-bar--error' : ''}`}>
      {message ?? 'Nessun errore.'}
    </p>
  )
}
