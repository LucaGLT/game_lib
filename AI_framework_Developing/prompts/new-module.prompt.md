---
mode: agent
description: "Generate a complete PLAN.md for a new module/library from scratch."
---

# New Module Plan

Generate a complete `PLAN.md` for a new `{{PROJECT_NAME}}` module/library.

Follow `.github/instructions/planning.instructions.md` exactly.
Validate the output against `.github/specs/plan-schema.yml`.
Follow all naming and style rules in `.github/specs/coding-style.yml`.

---

## Input Required

Answer these questions before generating:

1. **Module name** — the short name (e.g. `{{MODULE_NAME}}`).
2. **Namespace** — same as module name, following `{{NAMESPACE_PATTERN}}`.
3. **Purpose** — one sentence describing what the module does.
4. **Inspired by** — is this module modelled after an existing one? If yes, which?
5. **Key components** — list the main classes or interfaces expected (e.g. facade, strategy, router).
6. **Number of planned phases** — how many phases of development are expected?
7. **Phase 1 scope** — what must Phase 1 (Interfaces & Stubs) deliver?

---

## Output Requirements

Generate a single Markdown file named `PLAN.md` inside `{{MODULE_ROOT_PATH}}`.

The file must contain, in order:

1. Header block with all four mandatory fields: version `0.1`, status `Phase 1 – Planned ⏳`,
   language `{{LANGUAGE_STANDARD}}`, namespace matching the module name.
2. Goal section (3–6 sentences) with a comparison table if the module is modelled after another.
3. Architecture diagram (ASCII or Mermaid).
4. File structure tree with `←` role annotations for every file.
5. Development phases:
   - Phase 1: Interfaces & Stubs (all items unchecked).
   - Phase 2 onward: one phase per logical milestone.
   - Each phase ends with a smoke test item.
   - Each phase includes a `**Notes:**` block (bullet list) explaining implementation decisions.
6. (Optional) Key Design Decisions section.

Do not generate any source code. Do not generate `ai-instructions.md` or API docs.
