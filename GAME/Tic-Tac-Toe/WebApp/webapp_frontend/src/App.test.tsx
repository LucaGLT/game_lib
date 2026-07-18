import { describe, expect, it } from 'vitest'
import { render, screen } from '@testing-library/react'
import App from './App'

describe('App (Phase 3 themed board)', () => {
  it('renders the New Game button and the waiting turn header', () => {
    render(<App />)

    expect(screen.getByRole('button', { name: 'Nuova Partita' })).toBeInTheDocument()
    expect(screen.getByText("In attesa dell'inizio della partita…")).toBeInTheDocument()
  })

  it('renders 9 disabled board cells before any session is created', () => {
    render(<App />)

    const cells = screen.getAllByRole('button', { name: /Cella riga/ })
    expect(cells).toHaveLength(9)
    for (const cell of cells) {
      expect(cell).toBeDisabled()
    }
  })

  it('shows both player badges in idle state', () => {
    render(<App />)

    expect(screen.getByText('Player X: in attesa')).toBeInTheDocument()
    expect(screen.getByText('Player O: in attesa')).toBeInTheDocument()
  })
})
