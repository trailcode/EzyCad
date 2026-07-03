# Optional midpoint nodes on new linear edges (Line / Multi-Line tools)

**Suggested labels:** `enhancement`, `sketch`, `ui`, `docs`

---

## Title (GitHub)

Add setting to control automatic midpoint nodes on new linear edges (Line Edge and Multi-Line Edge tools)

## Body (GitHub)

### Summary

Add a user preference **"Add midpoints to new linear edges"** (default **off**) that controls whether the **Add line edge** and **Add multi-line edge** tools create automatic midpoint nodes on each new straight segment. The setting is available in **Settings > Sketch** and as **"Add midpoint nodes"** in the Options pane while those tools are active. JSON sketch load/save supports linear edges with or without midpoint nodes for compatibility.

### Problem

- Every new straight edge previously received an automatic midpoint node for center snapping, whether or not the user wanted it.
- Extra midpoint nodes add topology clutter and can make sketches harder to read or edit.
- `docs/bugs.md` tracked "Need a option to not add edge midpoint nodes" as an open wish.
- Users who rely on midpoint snapping had no way to turn the behavior off globally; users who do not need mids had no way to opt out without manual cleanup.

### Implemented scope (for drafts after work begins)

**Code changes:**

- `src/sketch.cpp` / `src/sketch.h`: static `Sketch::set_add_mid_pt_edges` / `get_add_mid_pt_edges`; `update_edge_end_pt_` only creates a midpoint node when the flag is on (`node_idx_mid` is `std::nullopt` when off).
- `src/gui.h` / `src/gui_settings.cpp`: `m_add_mid_pt_edges` member (default `false`); persisted as `gui.add_mid_pt_edges` in settings JSON; legacy read fallback for `add_midpoints_to_linear_edges`.
- `src/gui_settings.cpp`: Settings > Sketch checkbox **"Add midpoints to new linear edges"** with tooltip.
- `src/gui_mode.cpp`: Options pane checkbox **"Add midpoint nodes"** for Line Edge and Multi-Line Edge modes (syncs with the global setting).
- `src/gui.cpp`: applies setting before `finalize_elm()` when finishing a line/multi-line placement.
- `src/sketch_json.cpp`: linear edges serialize as `[a, b]` when no mid, `[a, b, mid]` when present; load accepts both forms.
- `res/ezycad_settings.json`: default `add_mid_pt_edges: false`.

**Documentation:**

- `docs/usage-settings.md`: Sketch section bullet for the new Settings control.
- `docs/usage-sketch.md`: updated Sketch snapping Tips, Line Edge Tips, and Add node section to describe optional mids (default off) and link to Settings.
- `CHANGELOG.md`: user-focused entry under [Unreleased] / Sketch / 2D Topology.
- `docs/bugs.md`: struck through the "not add edge midpoint nodes" wish (implemented).

**Tests:**

- `tests/sketch_tests.cpp`: fixture forces mids on for backward compatibility with existing tests; new `AddLinearEdge_MidpointOption` test covers off/on behavior and JSON shape; explicit `set_add_mid_pt_edges(true)` in tests that require mids.

**Behavior notes (for reviewers):**

- Setting affects only **new** edges from Line / Multi-Line tools. Existing edges and mids loaded from `.ezy` are unchanged.
- Edge splitting on intersection still creates midpoint nodes on **split sub-segments** (topology maintenance path in `sketch.cpp`); this is separate from the new-edge creation path.
- Other tools (rectangle, square, slot, add node mid snap, etc.) are unchanged.

### Acceptance criteria

- [ ] Default is **off**: new linear edges from Line / Multi-Line tools have no automatic midpoint node.
- [ ] When enabled (Settings or Options), new linear edges get midpoint nodes as before.
- [ ] Setting persists across sessions via `gui.add_mid_pt_edges`.
- [ ] JSON round-trip works for edges with and without mids.
- [ ] User guide and changelog accurately describe default-off behavior.
- [ ] Tests pass (`EzyCad_tests`, especially `AddLinearEdge_MidpointOption` and existing topology tests).
- [ ] Build succeeds (native; WASM if applicable).

### Files touched

- `src/sketch.cpp`, `src/sketch.h`
- `src/sketch_json.cpp`
- `src/gui.cpp`, `src/gui.h`, `src/gui_mode.cpp`, `src/gui_settings.cpp`
- `res/ezycad_settings.json`
- `tests/sketch_tests.cpp`
- `docs/usage-settings.md`, `docs/usage-sketch.md`
- `CHANGELOG.md`, `docs/bugs.md`
- `agents/drafts/issues/archive/gh-138-edge-midpoint-option.md` (this draft)

### Related

- `docs/bugs.md` wish list item (now struck through).
- Prior sketch/topology docs: `agents/drafts/issues/active/gh-134-linear-edge-automatic-splitting.md`, issue #134 / PR #133 (2D topology thread).
- `agents/conventions/user-docs-sync.md` — settings + persisted key checklist.
- GitHub issue: https://github.com/trailcode/EzyCad/issues/138
- GitHub PR: https://github.com/trailcode/EzyCad/pull/139

### Test plan (for implementation drafts)

- [ ] `EzyCad_tests --gtest_filter="Sketch_test.AddLinearEdge_MidpointOption"` — off/on toggle, node counts, JSON array sizes.
- [ ] Run broader sketch topology tests (`*AddSquareThenMidEdge*`, `*SplitEdge*`, crossing tests) — fixture keeps mids on for compatibility.
- [ ] Manual GUI:
  - Fresh install / cleared settings: draw a line with default — no mid snap target at edge center.
  - Enable **Settings > Sketch > Add midpoints to new linear edges** — new lines get center snap.
  - Toggle **Add midpoint nodes** in Options while in Line / Multi-Line mode — behavior matches Settings.
  - Save/reload `.ezy` with edges that have and lack mids.
- [ ] `sphinx-build -b html -W docs docs/_build` and spot-check usage-sketch / usage-settings.
- [ ] Optional pre-merge doc gap: add `gui.add_mid_pt_edges` row to the JSON table in `usage-settings.md` and mention the Options checkbox under the sketch Options section (per `agents/conventions/user-docs-sync.md`).

---

**Post-creation notes:**

- GitHub issue: https://github.com/trailcode/EzyCad/issues/138
- GitHub PR: https://github.com/trailcode/EzyCad/pull/139
- Consider a clearer commit message than "Fix." when squashing or amending before merge.
