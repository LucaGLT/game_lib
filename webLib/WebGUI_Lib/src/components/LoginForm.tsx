/**
 * LoginForm — generic username/password form for eng_serve-style pilot-grade
 * auth. Game-agnostic: only calls the injected `onSubmit`, no direct
 * dependency on `AuthProvider` (keeps it independently reusable/testable).
 */
import { useState, type FormEvent } from 'react'

export interface LoginFormProps {
  onSubmit: (username: string, password: string) => Promise<void>
  title?: string
}

export function LoginForm({ onSubmit, title = 'Accedi' }: LoginFormProps) {
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [isSubmitting, setIsSubmitting] = useState(false)
  const [errorMessage, setErrorMessage] = useState<string | null>(null)

  async function handleSubmit(event: FormEvent<HTMLFormElement>): Promise<void> {
    event.preventDefault()
    setErrorMessage(null)
    setIsSubmitting(true)
    try {
      await onSubmit(username, password)
    } catch (caught) {
      setErrorMessage(caught instanceof Error ? caught.message : String(caught))
    } finally {
      setIsSubmitting(false)
    }
  }

  return (
    <form className="gmgui-login-form" onSubmit={(event) => void handleSubmit(event)}>
      <h2>{title}</h2>
      <label className="gmgui-login-form__field">
        Utente
        <input
          type="text"
          value={username}
          onChange={(event) => setUsername(event.target.value)}
          autoComplete="username"
          required
        />
      </label>
      <label className="gmgui-login-form__field">
        Password
        <input
          type="password"
          value={password}
          onChange={(event) => setPassword(event.target.value)}
          autoComplete="current-password"
          required
        />
      </label>
      <button type="submit" disabled={isSubmitting}>
        {isSubmitting ? 'Accesso in corso…' : 'Accedi'}
      </button>
      {errorMessage !== null && <p className="gmgui-login-form__error">{errorMessage}</p>}
    </form>
  )
}
