/**
 * JoinSessionForm — generic "enter a join code" form for shared-multiplayer
 * eng_serve sessions. Game-agnostic: only calls the injected `onSubmit` with
 * the raw code the user typed, no direct dependency on restClient/AuthProvider
 * (keeps it independently reusable/testable, same convention as LoginForm).
 */
import { useState, type FormEvent } from 'react'

export interface JoinSessionFormProps {
  onSubmit: (joinCode: string) => Promise<void>
  title?: string
}

export function JoinSessionForm({ onSubmit, title = 'Entra in una partita' }: JoinSessionFormProps) {
  const [joinCode, setJoinCode] = useState('')
  const [isSubmitting, setIsSubmitting] = useState(false)
  const [errorMessage, setErrorMessage] = useState<string | null>(null)

  async function handleSubmit(event: FormEvent<HTMLFormElement>): Promise<void> {
    event.preventDefault()
    setErrorMessage(null)
    setIsSubmitting(true)
    try {
      await onSubmit(joinCode.trim())
      setJoinCode('')
    } catch (caught) {
      setErrorMessage(caught instanceof Error ? caught.message : String(caught))
    } finally {
      setIsSubmitting(false)
    }
  }

  return (
    <form className="gmgui-join-session-form" onSubmit={(event) => void handleSubmit(event)}>
      {title !== '' && <h3>{title}</h3>}
      <label className="gmgui-join-session-form__field">
        Codice partita
        <input
          type="text"
          value={joinCode}
          onChange={(event) => setJoinCode(event.target.value.toUpperCase())}
          placeholder="ES. AB3XQ9"
          autoComplete="off"
          required
        />
      </label>
      <button type="submit" disabled={isSubmitting || joinCode.trim() === ''}>
        {isSubmitting ? 'Ingresso in corso…' : 'Entra'}
      </button>
      {errorMessage !== null && <p className="gmgui-join-session-form__error">{errorMessage}</p>}
    </form>
  )
}
