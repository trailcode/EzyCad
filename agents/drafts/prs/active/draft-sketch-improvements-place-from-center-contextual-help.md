# PR - sketch improvements (Trailcode/sketch_improvements)

## Title

Sketch: Add line edge "Place from center"; unify contextual ? help (GUI_DOC_HELP_ macro, verbosity >= 5)

## Summary

- **Place from center** (Add line edge Options only): first click sets the edge midpoint; second click or Tab length uses the **full** edge length. Preview and dimension input respect full span. Documented in `usage-sketch.md` with **?** help linking to Read the Docs.
- **Unified contextual help**: replace scattered `(?)` / hover-only tooltips with `doc_help_button_` + `GUI_DOC_HELP_(tooltip, doc_url)` macro. ImGui widget IDs come from `PushID(__func__)` + `PushID(__LINE__)` at the call site — no manual `"?##..."` strings.
- **Help visibility**: `ui_show_contextual_help()` when `gui.ui_verbosity >= 5` (default 6). Toolbar tooltips still use tier 1 only.
- **Layout**: **?** buttons consistently on the **right** of the control (checkbox, slider, drag field, combo).
- **`doc_urls` namespace** in `gui.h` for shared Read the Docs anchors (view roll, sketch snapping, line-edge options, OCCT view, startup project, etc.).
- **Tests**: `AddEdgeFromCenter_SecondClickFullSpan`, `AddEdgeFromCenter_DimensionUsesFullLength`.

Builds on the optional midpoint-nodes work merged in PR #139 (#138).

## Files Changed

- `src/sketch.cpp`, `src/sketch.h` — center-first edge placement; static `set_edge_from_center` / `get_edge_from_center`
- `src/gui.h` — `doc_urls`, `ui_show_contextual_help()`, `GUI_DOC_HELP_` / `GUI_DOC_HELP_SAME_LINE_` macros; `doc_help_button_` signature
- `src/gui.cpp` — `doc_help_button_` implementation; underlay panel help migrated
- `src/gui_mode.cpp` — Place from center checkbox; Options/line-edge help via `GUI_DOC_HELP_`
- `src/gui_settings.cpp` — bulk migration of Settings rows to `GUI_DOC_HELP_`
- `tests/sketch_tests.cpp` — Place from center unit tests
- `docs/usage-sketch.md` — Line edge Options section and workflow updates
- `docs/usage-settings.md` — UI verbosity + **?** button documentation
- `CHANGELOG.md` — [Unreleased] Place from center entry
- `agents/drafts/issues/active/draft-line-edge-place-from-center-contextual-help.md` (issue draft)
- `agents/drafts/prs/active/draft-sketch-improvements-place-from-center-contextual-help.md` (this PR draft)

Also on branch (agent notes from prior work, already merged separately): `agents/drafts/issues/archive/gh-138-edge-midpoint-option.md`, `agents/drafts/prs/archive/gh-139-edge-midpoint-option.md`.

## Related

- Issue: _(fill after `gh issue create`)_
- Branch: `Trailcode/sketch_improvements`
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/sketch_improvements
- Depends on / follows: #138 / PR #139 (optional midpoint nodes)
- Related help UI: #131 / PR #132
- `agents/conventions/user-docs-sync.md`

## Test Plan

- [ ] Build `EzyCad_lib` (Release) and `EzyCad.exe`.
- [ ] `EzyCad_tests --gtest_filter="Sketch_test.AddEdgeFromCenter_*"`
- [ ] Manual — Add line edge:
  - **Place from center** on: center click, symmetric second click, Tab full length, preview span.
  - Off: unchanged two-click behavior.
  - **Add midpoint nodes** and **Place from center** **?** buttons (right of checkbox) open correct docs.
- [ ] Manual — Settings (default verbosity 6):
  - View rotation step, grid extent X/Y, snap guide mode — **?** after control, tooltip + link.
  - UI verbosity 4: no contextual **?** buttons.
- [ ] Manual — Options: selection mode, orthographic, sketch snap rows — same help pattern.
- [ ] `sphinx-build -b html -W docs docs/_build`; review `#line-edge-options` and usage-settings verbosity bullet.
- [ ] No regressions in line/multi-line edge drawing, snap, or dimension display.

## Notes

- **Place from center** is session/tool Options only (not persisted in settings JSON); midpoint option remains in Settings + Options as in #139.
- `GUI_DOC_HELP_` must only be used inside `GUI` member functions (calls private `doc_help_button_`).
- Commits on branch: `Improvements.`, `WIP`, `Refactor` — recommend a descriptive squash message at merge.
- No WASM-specific changes expected; native build verified locally.

## Post-merge (checklist for the author)

- [ ] Fill GitHub issue/PR URLs in `agents/drafts/issues/active/draft-line-edge-place-from-center-contextual-help.md` and this file.
- [ ] Close linked GitHub issue if fully addressed.
- [ ] Confirm `CHANGELOG.md` [Unreleased] entry is complete.
- [ ] Optional follow-up: persist `edge_from_center` in settings if users want it sticky across sessions.
