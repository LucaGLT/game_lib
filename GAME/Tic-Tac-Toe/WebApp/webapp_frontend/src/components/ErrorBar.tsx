interface ErrorBarProps {
  message: string | null
}

/** Bottom message bar — mirrors the desktop `ErrorBarWidget` (idle vs error state). */
function ErrorBar({ message }: ErrorBarProps) {
  const isError = message !== null
  return (
    <p className={`error-bar${isError ? ' error-bar--error' : ''}`}>
      {message ?? 'Nessun errore.'}
    </p>
  )
}

export default ErrorBar
