---
applyTo: "**/*.{cpp,hpp,h,c,cc,cxx}"
---

# Code Review Instructions — game_lib

> Apply these instructions when asked to review existing C++ code for compliance
> with project rules. The output is always a structured `REVIEW.md` correction plan,
> not inline edits (unless the user explicitly asks for immediate fixes).

---

## Review Scope

A review analyses one or more files against **all** rules defined in:

- `.github/specs/cpp-style.yml` — naming, formatting, spacing, constraints
- `.github/instructions/cpp-style.instructions.md` — the same rules in prose form
- `.github/instructions/documentation.instructions.md` — Doxygen completeness

**Do not** review for correctness of logic or algorithm unless explicitly asked.
**Do not** apply fixes silently — produce the plan first.

---

## Review Process (step by step)

1. **Collect** the list of files to review (from user input or the whole library).
2. **Scan** each file systematically through each rule category (see checklist below).
3. **Record** every violation: rule ID, file, symbol or line, description, correction.
4. **Group** violations by rule category.
5. **Assign severity** to each finding (see severity scale below).
6. **Produce** a `REVIEW.md` file following `.github/specs/review-plan-schema.yml`.
7. **Summarise** findings in a short table at the top of the review.

Never stop at the first violation. Scan the entire requested scope before writing output.

---

## Rule Categories Checklist

Work through these categories in order. Mark each as ✅ (no violations) or 🔴 (violations found).

### CAT-1 · Naming
- [ ] Namespaces match `gm` + PascalCase pattern (NS-1)
- [ ] Classes/structs/enums use PascalCase, no Hungarian prefix (CL-1)
- [ ] Only the main façade carries `Gm` prefix (CL-2)
- [ ] Interfaces use `I` prefix (FILE-3)
- [ ] Functions and methods use snake_case (FN-1)
- [ ] Boolean queries have `is_`, `has_`, `can_` prefix (FN-2)
- [ ] Factory functions/methods use `create_` prefix (FN-3)
- [ ] Local variables and parameters use snake_case (VAR-1)
- [ ] Private members use `_` prefix only — not `m_` or `mX` (VAR-2)
- [ ] `constexpr` constants use SCREAMING_SNAKE_CASE (CONST-1)
- [ ] `enum class` enumerators use SCREAMING_SNAKE_CASE (CONST-2)
- [ ] Exception classes have `E` prefix; base has `Error` suffix (EX-1, EX-2)

### CAT-2 · Include Guards
- [ ] Every header uses `#ifndef` + `#define` + `#endif` (no `#pragma once`) (IG-1)
- [ ] Guard macro follows `<NAMESPACE_UPPER>_<FILENAME_UPPER>_HPP` pattern (IG-1)

### CAT-3 · Formatting
- [ ] Indentation uses real tabs, width 4 — no mixed tabs/spaces (FMT-1)
- [ ] Opening braces are on their own line (Allman) for all constructs (FMT-2)
- [ ] Lines do not exceed 100 columns (FMT-3)
- [ ] Pointer/reference `*`/`&` attached to the type (FMT-4)
- [ ] Single-statement blocks without `{}` are only truly trivial cases (FMT-5)

### CAT-4 · Spacing
- [ ] One space around binary operators (SPC-1)
- [ ] No space for unary operators (SPC-2)
- [ ] One space after comma, none before (SPC-3)
- [ ] Space before `(` in control statements; none in function calls (SPC-4)

### CAT-5 · Switch / Case
- [ ] `case` indented one level, statements two levels inside `switch` (SW-1)
- [ ] No implicit fallthrough — `[[fallthrough]];` used where intentional (SW-3)

### CAT-6 · Multiline Signatures
- [ ] Signatures > 100 cols use one parameter per line (SIG-1)
- [ ] Parameters aligned under opening `(` (SIG-2)
- [ ] Closing `)` on its own line for definitions; `{` on next line (SIG-3)

### CAT-7 · Includes
- [ ] Include order: own header, stdlib, third-party, project (INC-1)
- [ ] Project headers use `"..."`, stdlib/third-party use `<...>` (INC-2)
- [ ] No `..` in relative include paths (INC-2)

### CAT-8 · Preprocessor
- [ ] All `#if`/`#else`/`#endif` start at column 1 (PP-1)
- [ ] Complex `#endif` blocks have trailing comments (PP-2)

### CAT-9 · Documentation
- [ ] All public classes have a Doxygen `@brief` block (DOC-1)
- [ ] All public methods have `@brief`, `@param`, `@return`, `@throws` where applicable (DOC-1)
- [ ] `enum class` values are documented with `///<` or a full block (DOC-1)
- [ ] Every header has a `@file` + `@brief` file-level block (DOC-1)
- [ ] All comments are in English (DOC-2)

### CAT-10 · Language Constraints
- [ ] No C++20 features used (`std::span`, `std::ranges`, concepts, etc.)
- [ ] No `auto` as return type in public prototypes
- [ ] No external library dependencies
- [ ] No cross-library `#include` outside `bridges/`
- [ ] `TODO` markers follow `// TODO: Phase N — description` format

---

## Severity Scale

| Level    | Code | Meaning                                                            |
|----------|------|--------------------------------------------------------------------|
| Error    | 🔴   | Direct violation of a mandatory rule. Must be fixed.               |
| Warning  | 🟡   | Deviation from a strong recommendation. Should be fixed.           |
| Info     | 🔵   | Minor inconsistency or missing optional element. Fix when possible.|

---

## Output: REVIEW.md

Follow `.github/specs/review-plan-schema.yml` for the exact structure.

Key constraints for the output:
- Every finding has a **rule ID** (e.g. `VAR-2`), **location** (file + symbol/line range),
  a **description** of the problem, and a **concrete correction** showing the fixed code.
- Group all findings under their category heading.
- Include a **summary table** at the top (counts per category and total).
- Include a **correction plan** section: ordered list of files to touch, grouped by severity.
- Do NOT include findings that are not real violations — no false positives.
