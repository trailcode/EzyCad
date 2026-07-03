# Line edge "Place from center" + unified contextual ? help (Settings / Options)

**Suggested labels:** `enhancement`, `sketch`, `ui`, `docs`

---

## Title (GitHub)

Add line edge "Place from center" option and unify contextual ? help buttons across Settings and Options (GUI_DOC_HELP_ macro)

## Body (GitHub)

### Summary

Extend the **Add line edge** tool with a **Place from center** Options checkbox: the first click sets the edge midpoint, and the second click or Tab length input uses the **full** edge length (symmetric placement). In the same pass, replace ad-hoc `(?)` labels and one-off `SmallButton("?")` / hover-only tooltips with a shared **`doc_help_button_`** helper and **`GUI_DOC_HELP_`** macro (ImGui ID from `__func__` + `__LINE__` via `PushID`). Contextual help is shown when **`gui.ui_verbosity` >= 5** (default **6**). Help buttons sit to the **right** of their control (checkbox, slider, combo, etc.) for consistent layout.

### Problem

- Users drawing symmetric segments (e.g. a vertical divider through a rectangle center) had to place one endpoint, then type **half** the full length or snap manually — error-prone and unlike CAD tools that support center-first placement.
- Contextual help in Settings and Options was inconsistent: some rows used disabled `(?)` text, some used hover-only tooltips on the control, some had manual `"?##foo_help"` ImGui IDs, and help visibility used mixed `ui_show_help(2)` / `ui_show_help(3)` tiers.
- Line edge Options for **Add midpoint nodes** lacked Read the Docs links matching the user guide sections added in PR #139.
- No single place for doc URL constants tied to help buttons.

### Implemented scope (for drafts after work begins)

**Sketch / line edge — Place from center:**

- `src/sketch.cpp` / `src/sketch.h`: `Sketch::set_edge_from_center` / `get_edge_from_center`; center-first placement in `move_line_string_pt_`, `complete_edge_from_center_`, dimension preview (`check_dimension_seg_` uses full length when active).
- `src/gui.h` / `src/gui_mode.cpp`: `m_edge_from_center`; **Place from center** checkbox in Add line edge Options (with **?** link to usage guide).
- `tests/sketch_tests.cpp`: `AddEdgeFromCenter_SecondClickFullSpan`, `AddEdgeFromCenter_DimensionUsesFullLength`.

**Contextual help refactor:**

- `src/gui.h`: `k_gui_ui_contextual_help_min_verbosity = 5`; `ui_show_contextual_help()`; `doc_urls` namespace (view roll, sketch snapping, line-edge options, OCCT view, startup project, etc.); `GUI_DOC_HELP_(tooltip, doc_url)` and `GUI_DOC_HELP_SAME_LINE_(...)` macros; private `doc_help_button_(scope, line, tooltip, doc_url, trailing_same_line)`.
- `src/gui.cpp`: `doc_help_button_` uses `ImGui::PushID(scope)` + `PushID(line)` and visible label `"?"`; gates on `ui_show_contextual_help()`.
- `src/gui_settings.cpp`, `src/gui_mode.cpp`, `src/gui.cpp`: migrate Settings rows (view nav, grid, dimensions, snap guides, startup project, underlay import, etc.) and Options rows (selection mode, orthographic, snap guide mode, co-axial nodes, line/multi-line midpoints) to `GUI_DOC_HELP_`. Replaced `ui_show_help(2|3)` with `ui_show_contextual_help()` for contextual UI; toolbar tooltips remain `ui_show_help(1)`.
- Help buttons consistently placed **after** the control (`SameLine` then `GUI_DOC_HELP_`).

**Documentation:**

- `docs/usage-sketch.md`: **Line edge Options** section (`#line-edge-options`, anchors for add-midpoint-nodes and place-from-center); workflow and tips updated.
- `docs/usage-settings.md`: UI verbosity note (5–6 show **?** help); `(?)` references updated to **?** buttons.
- `CHANGELOG.md`: [Unreleased] entry for Place from center.

### Acceptance criteria

- [ ] **Place from center** off (default): Add line edge behaves as before (first click = start point).
- [ ] **Place from center** on: first click = midpoint; second click or Tab length = full span; rubber-band preview shows full edge.
- [ ] **?** buttons appear in Settings and Options when `ui_verbosity >= 5`; hidden below that.
- [ ] **?** on hover shows tooltip; click opens Read the Docs when a URL is provided.
- [ ] Help layout: **?** to the right of the associated control (not between label and field).
- [ ] User guide and changelog match in-app behavior.
- [ ] Unit tests pass (`AddEdgeFromCenter_*`).
- [ ] Native build succeeds.

### Files touched

- `src/sketch.cpp`, `src/sketch.h`
- `src/gui.h`, `src/gui.cpp`, `src/gui_mode.cpp`, `src/gui_settings.cpp`
- `tests/sketch_tests.cpp`
- `docs/usage-sketch.md`, `docs/usage-settings.md`
- `CHANGELOG.md`
- `agents/drafts/issues/active/draft-line-edge-place-from-center-contextual-help.md` (this draft)

### Related

- Midpoint option (merged): issue #138, PR #139 — `agents/drafts/issues/archive/gh-138-edge-midpoint-option.md`, `agents/drafts/prs/archive/gh-139-edge-midpoint-option.md`
- Prior Options ? help pattern: issue #131, PR #132 — `agents/drafts/issues/archive/gh-131-sketch-tool-precise-input-docs-and-options-help.md`
- `agents/conventions/user-docs-sync.md`
- GitHub issue: _(fill after creation)_
- GitHub PR: _(fill after creation)_

### Test plan (for implementation drafts)

- [ ] `EzyCad_tests --gtest_filter="Sketch_test.AddEdgeFromCenter_*"`
- [ ] Manual Add line edge:
  - Enable **Place from center**; click center; second click symmetric; Tab length matches full span.
  - Toggle off; normal two-endpoint behavior.
  - **?** buttons on Options checkboxes open correct usage-sketch anchors.
- [ ] Settings (verbosity 6): spot-check view rotation step, grid extent X/Y, snap guide mode — **?** on right, tooltip + doc link.
- [ ] Lower UI verbosity to 4: contextual **?** buttons hidden.
- [ ] `sphinx-build -b html -W docs docs/_build`; review line-edge-options and usage-settings verbosity note.
- [ ] Release build `EzyCad.exe`.
