/**
 * useGmGuiModule — subscribes a handler to a module's declared typeIds on an
 * `EnvelopeRouter`, for the lifetime of the calling component. Web
 * equivalent of `MainWindow` registering an `IGmGuiModule` in its routing
 * table on attach and removing it on detach.
 */
import { useEffect, useRef } from 'react'
import type { EnvelopeRouter } from '../session/EnvelopeRouter'
import type { EnvelopeHandler } from '../session/types'
import type { GmGuiModuleDescriptor } from './GmGuiModule'

export function useGmGuiModule(
  router: EnvelopeRouter | null,
  descriptor: Pick<GmGuiModuleDescriptor, 'subscribedTypeIds'>,
  onEnvelope: EnvelopeHandler,
): void {
  // Keeps the latest handler in a ref so the effect below only resubscribes
  // when the router or the subscribed typeIds actually change, not on every
  // render (the caller's inline arrow function would otherwise churn it).
  const handlerRef = useRef(onEnvelope)
  handlerRef.current = onEnvelope

  const typeIdsKey = descriptor.subscribedTypeIds.join(',')

  useEffect(() => {
    if (router === null) {
      return undefined
    }
    return router.subscribe(descriptor.subscribedTypeIds, (envelope) => {
      handlerRef.current(envelope)
    })
    // typeIdsKey is the stable, by-value stand-in for descriptor.subscribedTypeIds.
  }, [router, typeIdsKey])
}
