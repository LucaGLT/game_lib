/**
 * AuthProvider — React context holding the current pilot-grade auth session
 * (Phase 2: Multi-Session & Auth). Generic across games: persists to
 * localStorage under a single game-agnostic key so every WebApp gets the
 * same "stay logged in after reload" behaviour for free, and validates the
 * restored token against `GET /auth/me` before trusting it.
 */
import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
  type ReactNode,
} from 'react'
import { fetchCurrentUser, login as loginRequest, logout as logoutRequest, type AuthSession } from './authClient'

const STORAGE_KEY = 'gmwebserve-auth-session'

export interface AuthContextValue {
  session: AuthSession | null
  isAuthenticated: boolean
  /** True while a stored session is being validated against the server on first mount. */
  isRestoring: boolean
  login: (username: string, password: string) => Promise<void>
  logout: () => void
}

const AuthContext = createContext<AuthContextValue | null>(null)

function loadStoredSession(): AuthSession | null {
  const raw = window.localStorage.getItem(STORAGE_KEY)
  if (raw === null) {
    return null
  }
  try {
    return JSON.parse(raw) as AuthSession
  } catch {
    return null
  }
}

export function AuthProvider({ children }: { children: ReactNode }) {
  const [session, setSession] = useState<AuthSession | null>(null)
  const [isRestoring, setIsRestoring] = useState(true)

  useEffect(() => {
    const stored = loadStoredSession()
    if (stored === null) {
      setIsRestoring(false)
      return
    }
    fetchCurrentUser(stored.token)
      .then((username) => {
        if (username === null) {
          window.localStorage.removeItem(STORAGE_KEY)
          return
        }
        setSession(stored)
      })
      .finally(() => setIsRestoring(false))
  }, [])

  const login = useCallback(async (username: string, password: string) => {
    const next = await loginRequest(username, password)
    window.localStorage.setItem(STORAGE_KEY, JSON.stringify(next))
    setSession(next)
  }, [])

  const logout = useCallback(() => {
    if (session !== null) {
      void logoutRequest(session.token)
    }
    window.localStorage.removeItem(STORAGE_KEY)
    setSession(null)
  }, [session])

  const value = useMemo<AuthContextValue>(
    () => ({ session, isAuthenticated: session !== null, isRestoring, login, logout }),
    [session, isRestoring, login, logout],
  )

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

/** Reads the current auth session. Must be used within an `AuthProvider`. */
export function useAuth(): AuthContextValue {
  const context = useContext(AuthContext)
  if (context === null) {
    throw new Error('useAuth must be used within an AuthProvider')
  }
  return context
}
