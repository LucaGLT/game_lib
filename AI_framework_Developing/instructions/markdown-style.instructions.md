---
applyTo: "**/*.md"
---

# Markdown Style Instructions — {{PROJECT_NAME}}

> These instructions apply to all Markdown files in the workspace.
> Use markdownlint conventions for formatting and structure.

---

## Scope

- Applies to `*.md` files only.
- Code style rules (indentation, brace style, line-length rules for code) do not apply to
  Markdown prose.

---

## Required Markdown Rules (markdownlint aligned)

1. Use spaces for indentation in Markdown lists and nested blocks. Do not use tab indentation.
2. Keep heading levels incremental and ordered (`#`, `##`, `###`, ...).
3. Use fenced code blocks and always specify a language when possible.
4. Surround lists with blank lines where required.
5. Keep table formatting consistent across each table.
6. Avoid trailing spaces at line end.
7. Use ordered lists only when order matters; otherwise use bullet lists.

---

## Practical Defaults

- Prefer `-` for unordered list items.
- Prefer compact, readable tables with consistent pipe alignment.
- Keep section titles concise and descriptive.
- Wrap prose naturally for readability; do not force source-code column rules onto Markdown text.

---

## Validation

- Before finalizing Markdown edits, validate against markdownlint rules.
- If no custom markdownlint config exists in the repo, follow markdownlint default rules.
