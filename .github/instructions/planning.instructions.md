---
applyTo: "**/PLAN.md"
---

# Planning Instructions — game_lib

> Apply these instructions whenever creating or updating a `PLAN.md` file.
> The YAML schema that defines the required structure is in `.github/specs/plan-schema.yml`.

---

## What a PLAN.md Must Contain

Every `PLAN.md` is the single source of truth for one library's development roadmap.
It must cover five mandatory sections (in order):

1. **Header block** — version, status, language, namespace.
2. **Goal** — one paragraph describing purpose and key design trade-offs.
3. **Architecture** — ASCII diagram showing component relationships.
4. **File Structure** — annotated directory tree with `←` role comments.
5. **Development Phases** — ordered list of phases; each phase has a checkbox list
   and a `**Notes:**` block that explains implementation decisions.

Optionally add a **Key Design Decisions** section (numbered list) after the phases.

---

## Header Block

```markdown
# <LibName> – Development Plan

**Version:** <semver>
**Status:** Phase <N> – <label> [✅ | 🔧 | ⏳]
**Language:** C++17 Standard
**Namespace:** `<namespace>`
```

Status labels:
- `Complete ✅` — all planned phases are done.
- `In progress 🔧` — at least one phase is currently being implemented.
- `Planned ⏳` — no phase has started yet.

---

## Goal Section

- One paragraph, 3–6 sentences.
- State the responsibility of the library in one sentence.
- Describe the main design choice (e.g. interface-based, policy-based, facade).
- If the library is modelled after another (`gmLog` → `gmDispatch`), include a
  comparison table.

---

## Architecture Section

Use an ASCII block diagram (fenced with triple backtick).
Show the call chain top-to-bottom: facade → internal engine → output components.
Add role annotations with `←`.

---

## File Structure Section

Use an annotated directory tree inside a triple-backtick block.
Every file or folder that has a defined role must have a `←` comment.

---

## Development Phases Section

### Phase Block Format

```markdown
### Phase N — <Title> [✅ | 🔧 | ⏳]

- [x] item already done
- [ ] item not yet done

**Notes:**
- Reason for a design decision made in this phase.
- Any gotcha or platform-specific issue discovered.
```

Rules:
- Phases are numbered from 1, never skip numbers.
- Phase 1 is always **Interfaces & Stubs**.
- Each phase ends with at least one integration/smoke test.
- Completed phases use `✅` and have all items checked `[x]`.
- Never delete a completed phase; keep the history.
- When adding a new phase, append it at the end; do not renumber existing phases.

---

## Updating an Existing PLAN.md

1. **Bump the version** (minor for new phase, patch for fixes/notes).
2. **Update `Status`** to reflect the current phase.
3. **Mark items done** with `[x]` as they are completed.
4. **Add `**Notes:**`** to the phase block when an implementation decision is made.
5. **Never rewrite history** — only append or annotate.

---

## What NOT to Put in PLAN.md

- Implementation code or pseudo-code.
- Instructions intended for a human developer (put those in `ai-instructions.md`).
- Per-class API documentation (put that in `*_API.md` or Doxygen blocks).
