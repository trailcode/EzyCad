# [Draft] UI improvements: Sketch/Shape list behavior and coding-style guidance

**Opened on GitHub from this draft.** Suggested labels: `enhancement`, `ui`, `docs`

**Branch / context:** `Trailcode/ui_improvements` (base: `main`)

---

## Title (GitHub)

UI improvements: Sketch/Shape list behavior and coding-style guidance

## Body (GitHub)

### Summary

This issue tracks follow-up and visibility for recent UI and style-guidance improvements from `Trailcode/ui_improvements`.

The branch improves list-pane usability in the GUI and clarifies coding-style guidance around reader-first organization and pragmatic DRY.

### Implemented scope (for tracking)

- Sketch List: selecting a sketch in the list switches GUI to sketch inspection mode.
- Sketch List and Shape List: improved narrow-width behavior with horizontal-scroll list content.
- Name field sizing: shared helper for list name input width (`GUI::list_name_field_width_`), used consistently by both lists.
- Readability/style updates:
  - Reader-first/top-down organization guidance for `.cpp` files.
  - Helper placement and local declaration guidance (near first use).
  - DRY guidance added as important but non-dogmatic.

### Why this matters

- Better day-to-day usability of Sketch/Shape list panes.
- More consistent UI behavior between both list panes.
- Clearer team guidance to reduce over-abstraction and keep code readable.

### Files touched (branch vs main)

- `src/gui.cpp`
- `src/gui.h`
- `src/gui_settings.cpp`
- `ezycad_code_style.md`

### Acceptance criteria

- [ ] Sketch List selection reliably enters sketch inspection mode.
- [ ] Sketch and Shape list panes behave consistently when resized narrow.
- [ ] Shared name-width logic remains centralized in one helper.
- [ ] Coding-style doc clearly captures reader-first ordering and pragmatic DRY guidance.
- [ ] Local AI assistant rule file mirrors the updated style guidance on contributor machines (these files are not committed).

### Links

- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/ui_improvements

---

## Metadata (do not paste into GitHub)

- Draft path: `agents/issues/003-ui-improvements-sketch-shape-lists-and-style.md`
- Repo: `trailcode/EzyCad`
