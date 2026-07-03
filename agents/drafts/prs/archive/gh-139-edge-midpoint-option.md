# PR - edge midpoint option (Trailcode/edge_mid_pt_option)

## Title

Sketch: optional midpoint nodes on new linear edges (Settings + Line/Multi-Line Options; default off)

## Summary

- Add **"Add midpoints to new linear edges"** (Settings > Sketch; default **off**) and a matching **"Add midpoint nodes"** checkbox in the Options pane for **Add line edge** and **Add multi-line edge** tools.
- When off, `update_edge_end_pt_` no longer creates automatic midpoint nodes on new straight edges (`node_idx_mid` is unset). When on, behavior matches the previous always-on default.
- Persist the preference as `gui.add_mid_pt_edges` (with legacy read for `add_midpoints_to_linear_edges`).
- JSON sketch serialization: linear edges without mids use a 2-element `[a, b]` array; edges with mids keep the 3-element `[a, b, mid]` form. Load accepts both.
- Update user guide, changelog, and bugs wish list; add unit test `AddLinearEdge_MidpointOption`. Test fixture forces mids on for existing tests that assume the old default.

Why the change was needed: automatic midpoint nodes on every linear edge added unnecessary topology for many workflows. Users asked for a way to disable them (`docs/bugs.md`).

Notable design decisions:

- Default **off** is a behavior change for new edges only; existing sketches and loaded mids are unaffected.
- Static `Sketch::set_add_mid_pt_edges` is synced from GUI settings and Options before edge finalization.
- Intersection/split topology maintenance still creates mids on split sub-edges (unchanged split path).
- Test fixture sets mids **on** so prior topology tests keep passing without rewriting every case.

## Files Changed

- `src/sketch.cpp`, `src/sketch.h` — midpoint option flag; conditional mid creation in `update_edge_end_pt_`
- `src/sketch_json.cpp` — optional mid in JSON load/save
- `src/gui.h`, `src/gui.cpp`, `src/gui_settings.cpp`, `src/gui_mode.cpp` — setting UI, persistence, Options checkboxes
- `res/ezycad_settings.json` — default `add_mid_pt_edges: false`
- `tests/sketch_tests.cpp` — `AddLinearEdge_MidpointOption`; fixture + targeted `set_add_mid_pt_edges(true)` for legacy tests
- `docs/usage-settings.md`, `docs/usage-sketch.md` — document optional mids (default off)
- `CHANGELOG.md` — [Unreleased] entry
- `docs/bugs.md` — strike through implemented wish
- `agents/drafts/issues/archive/gh-138-edge-midpoint-option.md` (issue draft)
- `agents/drafts/prs/archive/gh-139-edge-midpoint-option.md` (this PR draft)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/138
- PR: https://github.com/trailcode/EzyCad/pull/139
- Branch: `Trailcode/edge_mid_pt_option`
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/edge_mid_pt_option
- Related topology docs thread: issue #134, PR #133 (`agents/drafts/issues/active/gh-134-linear-edge-automatic-splitting.md`, `agents/drafts/prs/active/gh-133-document-sketch-edge-splitting-behavior.md`)
- `agents/conventions/user-docs-sync.md`

## Test Plan

- [ ] Build `EzyCad_lib` (Debug + Release) and `EzyCad_tests`.
- [ ] `EzyCad_tests --gtest_filter="Sketch_test.AddLinearEdge_MidpointOption"` — default off, toggle on, JSON 2- vs 3-element edge arrays.
- [ ] Run related sketch tests: `Sketch_test.EdgeManagement`, `AddSingleEdge_CreatesThreeCorrectNodes`, `SplitEdge_HasMidpoints`, `*AddSquareThenMidEdge*`, crossing/split tests.
- [ ] Manual verification:
  - With default settings, Line tool: new edge has 2 nodes only; no center snap to edge mid.
  - Enable in Settings or Options: new edges get center snap targets.
  - Toggle persists after restart.
  - Save `.ezy`, reload — edges without mids load correctly.
- [ ] Docs build (`sphinx-build -b html -W docs docs/_build`); review usage-sketch snapping/line sections and usage-settings Sketch bullet.
- [ ] Check in-app tooltips for Settings and Options checkboxes.
- [ ] Verify no regressions in snap, face splitting, or extrusion from divided sketches.
- [ ] (User-visible) cross-check `agents/conventions/user-docs-sync.md` — consider adding `gui.add_mid_pt_edges` to the JSON table and Options-panel sketch bullet if not done in this PR.

## Notes

- **Behavior change:** new installs and cleared settings no longer get automatic mids on Line / Multi-Line edges. Users who want center snapping must enable the setting.
- Split-on-intersection code still adds mids to sub-edges after a split (see `sketch.cpp` split loop). Documented in the issue draft; not changed in this PR.
- Minor whitespace alignment in `gui.cpp` underlay panel (unrelated formatting).
- Commit on branch is currently titled "Fix." — recommend a descriptive message at squash/merge time.
- CI: Windows native; no WASM-specific changes expected.

## Post-merge (checklist for the author)

- [ ] Update `agents/drafts/issues/archive/gh-138-edge-midpoint-option.md` with GitHub issue/PR numbers.
- [ ] Close linked GitHub issue if fully addressed.
- [ ] Confirm `CHANGELOG.md` [Unreleased] entry is accurate.
- [ ] Optional follow-up: complete JSON table + Options section entries in `usage-settings.md` per user-docs-sync checklist.
