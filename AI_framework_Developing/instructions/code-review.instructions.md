---
applyTo: "**/*.{{{LANGUAGE_FILE_EXTENSIONS}}}"
---

# Code Review Instructions — {{PROJECT_NAME}}

> Apply these instructions when asked to review existing `{{LANGUAGE}}` code for compliance
> with project rules. The output is always a structured `REVIEW.md` correction plan,
> not inline edits (unless the user explicitly asks for immediate fixes).

---

## Review Scope

A review analyses one or more files against **all** rules defined in:

- `.github/specs/coding-style.yml` — naming, formatting, spacing, constraints
- `.github/instructions/coding-style.instructions.md` — the same rules in prose form
- `.github/instructions/documentation.instructions.md` — doc-comment completeness

**Do not** review for correctness of logic or algorithm unless explicitly asked.
**Do not** apply fixes silently — produce the plan first.

---

## Review Process (step by step)

1. **Collect** the list of files to review (from user input or the whole module).
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
Categories flagged **(conditional)** only apply when relevant to `{{LANGUAGE}}` — skip them if
the language has no equivalent construct.

### CAT-1 · Naming

- [ ] Namespaces/modules match `{{NAMESPACE_PATTERN}}` (NS-1)
- [ ] Classes/structs/enums use the convention in `coding-style.yml` (CL-1)
- [ ] Only the main façade carries `{{FACADE_PREFIX}}` (CL-2, if facade pattern is used)
- [ ] Interfaces use `{{INTERFACE_PREFIX}}` prefix (FILE-3)
- [ ] Functions and methods use `{{FUNCTION_NAMING_CONVENTION}}` (FN-1)
- [ ] Boolean queries have `{{BOOLEAN_QUERY_PREFIXES}}` prefix (FN-2)
- [ ] Factory functions/methods use `{{FACTORY_PREFIX}}` prefix (FN-3)
- [ ] Local variables and parameters follow the registry convention (VAR-1)
- [ ] Private members follow `{{PRIVATE_MEMBER_CONVENTION}}` (VAR-2)
- [ ] Constants use `{{CONSTANT_NAMING_CONVENTION}}` (CONST-1)
- [ ] Enum values use `{{CONSTANT_NAMING_CONVENTION}}` (CONST-2)
- [ ] Exception classes have `{{EXCEPTION_PREFIX}}` prefix; base has `{{EXCEPTION_BASE_SUFFIX}}`
      suffix (EX-1, EX-2)

### CAT-2 · Structural Guards / Module Boundaries **(conditional)**

- [ ] Every header uses include guards, not `#pragma once`, if `{{USE_INCLUDE_GUARDS}}` (IG-1)
- [ ] Guard/module-boundary pattern follows the project convention (IG-1)

### CAT-3 · Formatting

- [ ] Indentation uses `{{INDENT_STYLE}}`, width `{{INDENT_WIDTH}}` — no mixed styles (FMT-1)
- [ ] Brace placement follows `{{BRACE_STYLE}}` for all constructs, if applicable (FMT-2)
- [ ] Lines do not exceed `{{LINE_LENGTH_LIMIT}}` columns (FMT-3)
- [ ] Pointer/reference symbols (if applicable to `{{LANGUAGE}}`) attached consistently (FMT-4)
- [ ] Single-statement blocks without braces are only truly trivial cases (FMT-5)

### CAT-4 · Spacing

- [ ] One space around binary operators (SPC-1)
- [ ] No space for unary operators (SPC-2)
- [ ] One space after comma, none before (SPC-3)
- [ ] Space before `(` in control statements; none in function calls, where the language
      convention applies (SPC-4)

### CAT-5 · Control Flow Constructs **(conditional)**

- [ ] `case`/`switch`-equivalent indentation matches the project convention (SW-1)
- [ ] No implicit fallthrough — explicit marker used where intentional (SW-3)

### CAT-6 · Multiline Signatures

- [ ] Signatures exceeding `{{LINE_LENGTH_LIMIT}}` columns use one parameter per line (SIG-1)
- [ ] Parameters aligned under the opening bracket (SIG-2)
- [ ] Closing bracket / opening brace placement follows `{{BRACE_STYLE}}` (SIG-3)

### CAT-7 · Includes / Imports

- [ ] Import/include order follows the project convention: own module, standard library,
      third-party, project (INC-1)
- [ ] Project vs. external import syntax used consistently (INC-2)
- [ ] No relative-parent (`..`) paths in imports where avoidable (INC-2)

### CAT-8 · Preprocessor / Build Directives **(conditional — only if `{{HAS_PREPROCESSOR}}`)**

- [ ] All directives start at column 1 (PP-1)
- [ ] Complex conditional blocks have trailing comments (PP-2)

### CAT-9 · Documentation

- [ ] All public classes have a `{{DOC_COMMENT_STYLE}}` brief block (DOC-1)
- [ ] All public methods document parameters, return value, and thrown/raised errors (DOC-1)
- [ ] Enum/constant values are documented at minimum with a short inline description (DOC-1)
- [ ] Every file has a file-level brief block (DOC-1)
- [ ] All comments are in English (DOC-2), unless the project explicitly overrides this

### CAT-10 · Language Constraints

- [ ] No use of language features beyond `{{LANGUAGE_STANDARD}}`
- [ ] No inferred/implicit return types in public prototypes, if disallowed by project convention
- [ ] No external dependencies, unless `{{ALLOW_EXTERNAL_DEPENDENCIES}}` is `true`
- [ ] No cross-module coupling outside the `{{CROSS_MODULE_POLICY}}`
- [ ] `TODO` markers follow `// TODO: Phase N — description` format (adapt comment syntax)

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
