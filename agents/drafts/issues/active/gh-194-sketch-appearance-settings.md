---
github_issue: 194
github_pr: 195
status: active
paired_draft: ../prs/active/gh-195-sketch-appearance-settings.md
---

# Configurable sketch edge/face colors and Settings header persistence

**Suggested labels:** `enhancement`, `ui`, `sketch`

---

## Title (GitHub)

Configurable sketch edge/face colors and Settings header persistence

## Body (GitHub)

### Summary

Make sketch edge and face appearance configurable (color, thickness, transparency), with separate colors for normal, selection, and mouse-over highlight. Persist Settings pane collapsing-header open/closed state.

### Problem

- Sketch edges and faces used hard-coded colors (green edges, GRAY7 faces, yellow OCCT highlight for both selection and hover).
- Users could not tune sketch appearance or transparency from Settings.
- Settings collapsing sections always reset to DefaultOpen / collapsed defaults on restart.

### Implemented scope

**Code changes:**

- `src/gui.h` / `src/gui_settings.cpp` — appearance members, JSON keys, Appearance UI, `settings_headers` persistence.
- `src/sketch_display.cpp` / `src/sketch.h` / `src/sketch.cpp` / `src/sketch_topo.cpp` — apply edge/face styles and selection vs hover drawers; `edge_face_style` refresh.

**Documentation:**

- `docs/usage-settings.md`, `docs/usage-sketch.md`, `CHANGELOG.md`, `src/doc/gui.md`, `src/doc/sketch.md`
- `res/ezycad_settings.json` bundled defaults

### Acceptance criteria

- [ ] Settings -> Sketch -> Appearance exposes edge/face color controls with alpha.
- [ ] Selection vs hover colors are distinct for edges and faces.
- [ ] Settings collapsing headers persist across restarts.
- [ ] Docs and CHANGELOG updated; Defaults restores bundled palette.
- [ ] Native build succeeds.

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/194
- PR: https://github.com/trailcode/EzyCad/pull/195
- Draft PR: `agents/drafts/prs/active/gh-195-sketch-appearance-settings.md`
