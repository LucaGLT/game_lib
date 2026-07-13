# PyQt Visual Theme Specification

## Purpose

This document defines the visual design system for all PyQt graphical user interfaces.

It intentionally contains no application-specific concepts.

The objective is to ensure:

- Visual consistency
- Theme switchability
- QSS-driven styling
- Widget reusability
- Domain-independent GUI design

This specification must be applied regardless of application type.

The GUI may represent:

- Games
- Business applications
- Editors
- Tools
- Simulators
- Utilities

without requiring modifications to the visual system.

---

## Fundamental Principle

Appearance must be completely separated from functionality

Widget code must never contain:

- hardcoded colors
- hardcoded fonts
- hardcoded border styles
- hardcoded visual effects

All visual customization must be performed through:

- QSS
- Theme variables
- Fonts
- Generic widget properties

---

## Global Design Rules

### Visual Density

Use medium density.

Avoid:

- crowded layouts
- excessive whitespace

Preferred spacing units:

4 px
8 px
16 px
24 px
32 px

Only use multiples of these values.

---

### Corner Radius Scale

Themes may define:

0 px
2 px
4 px
8 px
12 px
16 px

No arbitrary values.

---

### Border Thickness

Allowed values:

1 px
2 px
3 px

No thicker borders.

---

### Shadows

Shadows must remain subtle.

Avoid:

- large glow effects
- large drop shadows

Shadows are decorative accents, not structural elements.

---

### Typography

**Title**

Used for:

- dialog titles
- panel titles
- major section titles

Characteristics:

largest font
bold
high contrast

---

**Subtitle**

Used for:

- section headers
- categories
- group labels

Characteristics:

smaller than title
medium emphasis

---

**Body**

Used for:

- labels
- descriptions
- content

Characteristics:

high readability
normal weight

---

**Secondary Text**

Used for:

- hints
- timestamps
- metadata

Characteristics:

lower contrast
smaller size

---

### Generic Widget Styling

**Dialog**

Must provide:

- visual hierarchy
- clear content separation

Avoid:

- heavy decorations
- excessive gradients

---

**Panel**

Panels are the primary grouping element.

Must provide:

- subtle background differentiation
- visual containment

Panels should not visually dominate content.

---

**Label**

Labels must prioritize readability.

Avoid:

- decorative effects
- excessive color variation

---

**Push Button**

States:

Normal
Hover
Pressed
Disabled

All themes must define all states.

Primary actions should be visually emphasized.

Secondary actions should remain neutral.

Danger actions should clearly communicate risk.

---

**Toggle Button**

Must visually indicate:

ON
OFF

without requiring textual interpretation.

---

**Check Box**

Must clearly indicate:

Unchecked
Checked
Disabled

---

**Radio Button**

Must clearly indicate:

Selected
Unselected
Disabled

---

**Combo Box**

Must provide:

- visible dropdown indicator
- hover feedback
- focus feedback

---

**Text Box**

Must provide:

- focus indication
- disabled indication
- validation feedback

States:

Normal
Focused
Disabled
Error

---

**Spin Box**

Must visually integrate with:

- line edits
- combo boxes

---

**List View**

Must support:

Hover
Selected
Disabled

Selection must remain visible.

---

**Tree View**

Must provide:

- clear hierarchy
- visible expand/collapse indicators

Tree structure must remain readable even without icons.

---

**Table View**

Must support:

- row selection
- alternating rows
- sortable headers

Headers must remain visually distinct.

---

**Tab Widget**

States:

Inactive
Hover
Active

Active tab must be immediately recognizable.

---

**Scroll Bar**

Must be visually lightweight.

Avoid:

- oversized handles
- distracting colors

---

**Progress Bar**

Must clearly communicate progress.

Avoid unnecessary animation.

---

**Tool Tip**

Must be readable and compact.

Avoid large blocks of text.

---

**Visual States**

All themes must define:

Normal
Hover
Focused
Selected
Active
Disabled
Warning
Error
Success

These states must be visually consistent across all widgets.

---

## Theme Definitions

### Theme: Scroll

Concept

Ancient parchment.

Keywords:

Warm
Organic
Historical
Elegant
Readable

Colors

Background

#E8DFC8

Panel

#F1E8D0

Border

#7B6444

Primary Accent

#A8873A

Text

#2E2418

Radius

4 px

Mood

Ancient manuscript
Library
Archive
Scroll

---

### Theme: Stone

Concept

Carved stone.

Keywords:

Solid
Ancient
Monumental
Neutral

Colors

Background

#BDB8AF

Panel

#D2CEC6

Border

#59544D

Accent

#7A7A6B

Text

#1E1E1E

Radius

2 px

Mood

Fortress
Temple
Ruins
Mountain hall

---

### Theme: Dark Moon

Concept

Dark fantasy.

Keywords:

Moonlight
Ashes
Mystery
Decay

Colors

Background

#1A1A1E

Panel

#26262D

Border

#54546A

Accent

#A89CC8

Text

#E5E5E5

Radius

6 px

Mood

Forgotten kingdom
Moonlit ruins
Dark fantasy

---

### Theme: Blood

Concept

Corruption and sacrifice.

Keywords

Crimson
Ritual
Cathedral
Violence

Colors

Background

#140A0A

Panel

#241111

Border

#6B1515

Accent

#B52A2A

Text

#F2E6E6

Radius

8 px

Mood

Blood ritual
Vampiric court
Dark cathedral

---

### Theme: Techno

Concept

Futuristic control interface.

Keywords

Digital
Clean
Technical
Precise

Colors

Background

#08131E

Panel

#10202D

Border

#00C8FF

Accent

#00E5FF

Text

#D8F8FF

Radius

12 px

Mood

Control room
Starship
Cybernetic interface
Mission terminal

---

## Mandatory Rule For AI Systems

When generating PyQt interfaces:

1. Always use QSS.
2. Never hardcode colors.
3. Never hardcode fonts.
4. Never hardcode border styles.
5. Never mix business logic with styling.
6. Every visual decision must derive from the active theme.
7. Widgets must remain reusable across themes.
8. Theme switching must not require widget code modifications.
9. Domain information must never influence visual styling.
10. The same widget hierarchy must render correctly under every theme.


