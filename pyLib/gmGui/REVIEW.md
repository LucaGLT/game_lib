# gmGui Visual Compliance Review

## Scope

Review target:

- pyLib/gmGui/modules
- pyLib/gmGui/widgets
- pyLib/gmGui/main_window.py

Reference specifications used:

- .github/instructions/python-gui-style.instructions.md
- .github/specs/gui-theme.yml
- .github/GUI style.md

Date: 2026-06-18

---

## Findings (ordered by severity)

## CRITICAL-1 — No global theme system (QSS) detected

Rules impacted: GUI-1, GUI-6, GUI-8, GUI-10

Evidence:

- No .qss files found under pyLib/gmGui
- No ThemeManager-like component found in pyLib/gmGui/main_window.py

Impact:

- There is no single source of visual truth.
- Theme switching across the 5 mandated themes cannot be guaranteed.
- Visual behavior is currently coupled to widget code.

Corrective action:

- Introduce a centralized theme subsystem (for example a ThemeManager) that applies app-level QSS.
- Move all visual tokens (colors, borders, typography, state selectors) into theme definitions.
- Ensure runtime theme switching updates the whole UI with no widget-code edits.

---

## CRITICAL-2 — Hardcoded visual values in Python modules

Rules impacted: GUI-2, GUI-3, GUI-4, GUI-6

Evidence:

- pyLib/gmGui/modules/gm_actor_module.py:88
- pyLib/gmGui/modules/gm_actor_module.py:164
- pyLib/gmGui/modules/gm_actor_module.py:172
- pyLib/gmGui/modules/gm_actor_module.py:177
- pyLib/gmGui/modules/gm_actor_module.py:182
- pyLib/gmGui/modules/gm_actor_module.py:197
- pyLib/gmGui/modules/gm_actor_module.py:214
- pyLib/gmGui/modules/gm_dice_module.py:168
- pyLib/gmGui/modules/gm_dice_module.py:249
- pyLib/gmGui/modules/gm_dice_module.py:270
- pyLib/gmGui/modules/gm_dice_module.py:321
- pyLib/gmGui/modules/gm_dice_module.py:326
- pyLib/gmGui/modules/gm_flow_module.py:90
- pyLib/gmGui/modules/gm_flow_module.py:95
- pyLib/gmGui/modules/gm_flow_module.py:101
- pyLib/gmGui/modules/gm_flow_module.py:106
- pyLib/gmGui/modules/gm_flow_module.py:123
- pyLib/gmGui/modules/gm_flow_module.py:160
- pyLib/gmGui/modules/gm_flow_module.py:180
- pyLib/gmGui/modules/gm_flow_module.py:192

Impact:

- Color/font/border decisions are embedded in code.
- Portability across Scroll/Stone/Dark Moon/Blood/Techno themes is blocked.

Corrective action:

- Replace inline setStyleSheet() literals with objectName/property-based selectors in global QSS.
- Replace all hardcoded QFont setup in modules with typography roles managed by QSS.
- Convert state color changes to dynamic properties (for example state=alive/dying/dead) plus repolish.

---

## CRITICAL-3 — Domain-driven color logic in widget rendering

Rules impacted: GUI-2, GUI-6, GUI-9, WS-1

Evidence:

- pyLib/gmGui/widgets/map_scene.py:22
- pyLib/gmGui/widgets/map_scene.py:130
- pyLib/gmGui/widgets/timeline_scene.py:72
- pyLib/gmGui/widgets/timeline_scene.py:76
- pyLib/gmGui/widgets/timeline_scene.py:80
- pyLib/gmGui/widgets/hp_bar.py:65
- pyLib/gmGui/widgets/hp_bar.py:67
- pyLib/gmGui/widgets/hp_bar.py:68
- pyLib/gmGui/modules/gm_actor_module.py:42

Impact:

- Visual outcomes depend directly on domain values (faction, terrain, life_state, actor label X/O, HP ratio) using hardcoded colors.
- This prevents theme-level control and consistency of visual states.

Corrective action:

- For QWidget-based components: expose semantic Qt properties (variant, severity, life_state, faction_class) and style via QSS.
- For QGraphicsScene custom painting (where QSS cannot fully style primitives): route colors through a theme token provider instead of literals/QColor constants.
- Replace direct Qt.GlobalColor usage with resolved theme tokens.

---

## HIGH-1 — Spacing scale violations (non allowed values)

Rules impacted: SP-1

Evidence:

- pyLib/gmGui/modules/gm_dice_module.py:85 (6)
- pyLib/gmGui/modules/gm_dice_module.py:92 (6)
- pyLib/gmGui/modules/gm_dice_module.py:311 (3)
- pyLib/gmGui/modules/gm_dice_module.py:310 (mixed 8,6,8)
- pyLib/gmGui/modules/gm_comp_deck_module.py:98 (2)
- pyLib/gmGui/modules/gm_actor_module.py:168 (12,10,12)
- pyLib/gmGui/modules/gm_map_module.py:71 (4 ok) and 72 (4 ok) but verify all future edits keep only allowed scale

Impact:

- Layout density is not coherent with the mandatory spacing grid.
- Different modules render with inconsistent rhythm.

Corrective action:

- Normalize all margins/spacing to the allowed set or exact multiples of allowed scale as required by spec.
- Prefer 4/8/16 as base values for compact module UIs.

---

## HIGH-2 — Typography managed in code

Rules impacted: GUI-3, TY-1

Evidence:

- pyLib/gmGui/modules/gm_dice_module.py:155
- pyLib/gmGui/modules/gm_dice_module.py:166
- pyLib/gmGui/modules/gm_dice_module.py:175
- pyLib/gmGui/modules/gm_dice_module.py:216
- pyLib/gmGui/modules/gm_dice_module.py:254
- pyLib/gmGui/modules/gm_dice_module.py:269
- pyLib/gmGui/modules/gm_dice_module.py:317
- pyLib/gmGui/modules/gm_flow_module.py:122
- pyLib/gmGui/modules/gm_flow_module.py:179
- pyLib/gmGui/modules/gm_flow_module.py:353
- pyLib/gmGui/widgets/timeline_scene.py:61
- pyLib/gmGui/widgets/timeline_scene.py:94

Impact:

- Typography roles (title/subtitle/body/secondary) are not centrally enforceable.
- Theme consistency cannot be guaranteed.

Corrective action:

- Replace in-code QFont construction with role-based styling from global QSS (or tokenized painter style where needed in graphics scene text).

---

## HIGH-3 — Missing explicit multi-theme validation coverage

Rules impacted: GUI-10

Evidence:

- No evidence in current modules/widgets of tests validating rendering correctness across all 5 themes.

Impact:

- Regressions in visual states/themes may go undetected.

Corrective action:

- Add automated visual/state conformance tests for each module and core widget under all theme ids:
  scroll, stone, dark_moon, blood, techno.

---

## MEDIUM-1 — Border/radius values outside strict policy in inline styles

Rules impacted: BD-1, CR-1

Evidence:

- pyLib/gmGui/modules/gm_actor_module.py:88 (radius 8/10 mixed)
- pyLib/gmGui/modules/gm_actor_module.py:164 (radius 10)
- pyLib/gmGui/modules/gm_flow_module.py:95 (radius 4)
- pyLib/gmGui/modules/gm_dice_module.py:270 (radius 8)

Impact:

- Radius/border policy is not uniformly enforceable because values are embedded in local style strings.

Corrective action:

- Move radius and border thickness to theme tokens and enforce allowed scales in a single source.

---

## Action Plan (corrective)

## Phase 1 — Introduce theme infrastructure (blocking)

- Add a centralized theme manager for app-level stylesheet application.
- Create token maps per theme using .github/specs/gui-theme.yml.
- Add theme switch API (set_theme(theme_id)).

Deliverable:

- Theme can switch at runtime for the whole app without touching module code.

## Phase 2 — Refactor modules to semantic styling only

- gm_actor_module: remove inline style strings; use object names and dynamic properties.
- gm_dice_module: remove inline style and hardcoded fonts/colors.
- gm_flow_module: remove inline style and hardcoded fonts/colors.
- gm_comp_deck_module, gm_map_module: verify spacing policy and semantic object naming.

Deliverable:

- No hardcoded visual literals in module Python code.

## Phase 3 — Refactor graphics widgets to token-driven paint

- map_scene, timeline_scene, hp_bar: replace QColor literals with theme token resolution.
- Introduce semantic mapping from domain state to token key, not token value.

Deliverable:

- QGraphics-based drawing follows active theme tokens.

## Phase 4 — Grid and typography normalization

- Replace all non-conformant spacing values (2,3,6,10,12, etc.) with allowed scale policy.
- Replace direct QFont setup with typography-role resolution.

Deliverable:

- Full SP-1 and TY-1 conformance.

## Phase 5 — Verification and regression guard

- Add tests for theme switching and visual state consistency.
- Add checks for forbidden patterns (hex literals, QColor literals, inline setStyleSheet with visual values, hardcoded QFont setup).

Deliverable:

- Automated conformance gate for GUI-1..GUI-10, SP-1, TY-1, WS-1.

---

## Quick Win Priorities

1. Remove inline setStyleSheet() literals from gm_actor_module, gm_dice_module, gm_flow_module.
2. Introduce ThemeManager + global QSS load in main window bootstrap.
3. Normalize spacing violations in gm_dice_module and gm_comp_deck_module first.
4. Convert hp_bar and timeline_scene to theme-token painting.

---

## Assumptions

- This review focuses on visual compliance only (not gameplay/business correctness).
- QWidget controls should be QSS-driven; QGraphics custom paint should be token-driven.
- Existing tests currently prioritize behavior, not multi-theme visual conformance.
