/**
 * GmGuiModule — public contract for a generic, engine-driven UI module.
 * Web equivalent of `pyLib/gmGui/modules/base_module.py`'s `IGmGuiModule`:
 * each module declares which typeIds it needs and renders from its own
 * state, decoupled from every other module by the shared `EnvelopeRouter`.
 *
 * Unlike the desktop ABC (a QWidget-owning class instance), a web module is
 * just a React component using the `useGmGuiModule` hook — `moduleId`/`title`
 * stay useful for future generic shells (e.g. a dashboard listing active
 * modules), but the mount/unmount lifecycle itself is handled by React
 * instead of explicit `on_attach`/`on_detach` calls.
 */

export interface GmGuiModuleDescriptor {
  /** Unique key for this module (mirrors `IGmGuiModule.module_id`). */
  moduleId: string
  /** Human-readable title, e.g. for a future generic panel/dock shell. */
  title: string
  /** gmDispatch typeIds this module wants routed to it (`"*"` = every typeId). */
  subscribedTypeIds: readonly string[]
}
