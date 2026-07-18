import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { fireEvent, render, screen } from '@testing-library/react'
import { AuthProvider } from '@webgui/session/AuthProvider'
import App from './App'

function jsonResponse(body: unknown, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { 'Content-Type': 'application/json' },
  })
}

function renderApp() {
  return render(
    <AuthProvider>
      <App />
    </AuthProvider>,
  )
}

describe('App (Phase 2 login gate + Phase 3 themed board)', () => {
  beforeEach(() => {
    window.localStorage.clear()
  })

  afterEach(() => {
    vi.unstubAllGlobals()
  })

  it('renders the login form when not authenticated', () => {
    renderApp()

    expect(screen.getByRole('heading', { name: 'Tic-Tac-Toe — Accedi' })).toBeInTheDocument()
    expect(screen.getByLabelText('Utente')).toBeInTheDocument()
    expect(screen.getByLabelText('Password')).toBeInTheDocument()
  })

  it('shows an error message on failed login', async () => {
    vi.stubGlobal(
      'fetch',
      vi.fn().mockResolvedValue(jsonResponse({ detail: 'Credenziali non valide' }, 401)),
    )

    renderApp()
    fireEvent.change(screen.getByLabelText('Utente'), { target: { value: 'demo' } })
    fireEvent.change(screen.getByLabelText('Password'), { target: { value: 'wrong' } })
    fireEvent.click(screen.getByRole('button', { name: 'Accedi' }))

    expect(await screen.findByText('Credenziali non valide.')).toBeInTheDocument()
  })

  it('shows the session picker (no board yet) after a successful login', async () => {
    vi.stubGlobal(
      'fetch',
      vi.fn().mockImplementation((input: RequestInfo | URL) => {
        const url = String(input)
        if (url.includes('/auth/login')) {
          return Promise.resolve(
            jsonResponse({ token: 'test-token', username: 'demo', expires_at: 9999999999 }),
          )
        }
        if (url.includes('/sessions')) {
          return Promise.resolve(jsonResponse([]))
        }
        return Promise.resolve(jsonResponse({}))
      }),
    )

    renderApp()
    fireEvent.change(screen.getByLabelText('Utente'), { target: { value: 'demo' } })
    fireEvent.change(screen.getByLabelText('Password'), { target: { value: 'demo-pass' } })
    fireEvent.click(screen.getByRole('button', { name: 'Accedi' }))

    expect(await screen.findByRole('heading', { name: 'Sessioni attive' })).toBeInTheDocument()
    expect(
      screen.getByText(
        'Nessuna sessione attiva. Premi «Nuova Partita» per iniziare, oppure entra in una partita con un codice.',
      ),
    ).toBeInTheDocument()
    expect(screen.getByRole('button', { name: 'Nuova Partita' })).toBeInTheDocument()
    expect(screen.getByText('demo')).toBeInTheDocument()
  })
})
