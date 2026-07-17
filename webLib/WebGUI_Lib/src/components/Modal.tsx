/**
 * Modal — generic backdrop/chrome/focus wrapper for any dialog content, web
 * equivalent of PySide6's `QDialog` chrome (title bar, modal backdrop,
 * Esc/click-outside dismiss). Contains no domain-specific markup — game
 * content goes in `children`, exactly like every game's own dialogs
 * (`FormationModal`, `InstantWindowModal`, `MissionSelectModal`, ...) wrap
 * their content in this component instead of building their own backdrop.
 *
 * First consumer: Eldhôm's 3 interactive dialogs
 * (GAME/Eldhom/WebApp/PLAN.md, Phase 6). Mirrors the desktop pattern where
 * every `QDialog` subclass gets its modal chrome "for free" from Qt.
 */
import { useEffect, type ReactNode } from 'react'

export interface ModalProps {
  title: string
  /** Called on Escape or backdrop click. Omit for a non-dismissible (mandatory) modal — e.g. a formation dialog with no cancel option. */
  onDismiss?: () => void
  children: ReactNode
}

export function Modal({ title, onDismiss, children }: ModalProps) {
  useEffect(() => {
    if (!onDismiss) {
      return undefined
    }
    function handleKeyDown(event: KeyboardEvent): void {
      if (event.key === 'Escape') {
        onDismiss?.()
      }
    }
    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [onDismiss])

  return (
    <div
      className="gmgui-modal-backdrop"
      role="presentation"
      onClick={onDismiss ? () => onDismiss() : undefined}
    >
      <div
        className="gmgui-modal"
        role="dialog"
        aria-modal="true"
        aria-label={title}
        onClick={(event) => event.stopPropagation()}
      >
        <div className="gmgui-modal__header">
          <span className="gmgui-modal__title">{title}</span>
          {onDismiss && (
            <button
              type="button"
              className="gmgui-modal__close"
              onClick={onDismiss}
              aria-label="Chiudi"
            >
              ✕
            </button>
          )}
        </div>
        <div className="gmgui-modal__body">{children}</div>
      </div>
    </div>
  )
}
