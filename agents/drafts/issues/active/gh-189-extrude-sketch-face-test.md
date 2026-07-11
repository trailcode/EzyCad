---
github_issue: 189
github_pr: 188
status: active
paired_draft: ../prs/active/gh-188-unit-test-refactor.md
---

# Enable ExtrudeSketchFace_EzyCad and add Shp_extrude_access

**Suggested labels:** `sketch`, `refactor`

---

## Title (GitHub)

Enable ExtrudeSketchFace_EzyCad and add Shp_extrude_access for tests

## Body (GitHub)

### Summary

Enable the long-disabled `ExtrudeSketchFace_EzyCad` unit test, move extrude test hooks behind a `Shp_extrude_access` friend class (same pattern as `Sketch_access` / `View_access`), and clean up orphaned or stale comments left after the sketch test suite split.

### Problem

- `ExtrudeSketchFace_EzyCad` had been `#if 0` since the initial commit (`// Not working`).
- Re-enabling it failed because:
  - `Occt_view::sketch_face_extrude` now requires `(screen_coords, is_mouse_move)`.
  - Preview shapes are only committed to `get_shapes()` after `finalize()`.
  - AIS `MoveTo` face picking crashes / is unreliable on the headless offscreen test viewer.
- `Shp_extrude` exposed test-only methods on the production public API.
- After splitting `sketch_tests.cpp`, several comments were left above the wrong tests (or duplicated), including a stale note that midpoint crossing "currently" produced three edges.

### Implemented scope

**Code changes:**

- `Shp_extrude`: remove public test hooks; `friend class Shp_extrude_access`.
- `tests/sketch_test_fixture.*`: add `Shp_extrude_access::{set_curr_view_pln, begin_face_extrude}`; `View_access::set_view_plane` delegates to it.
- `tests/sketch_tests.cpp`: rewrite `ExtrudeSketchFace_EzyCad` to inject a known face, preview, finalize, and assert a solid.
- Clean orphaned / duplicate / stale comments in sketch test TUs.

### Acceptance criteria

- [x] `Sketch_test.ExtrudeSketchFace_EzyCad` compiles and passes (Debug).
- [x] No public test-only methods on `Shp_extrude`.
- [x] Orphaned / stale test comments removed or corrected.
- [ ] CI green on the linked PR.

### Files touched

- `src/shp_extrude.h`, `src/shp_extrude.cpp`
- `tests/sketch_test_fixture.h`, `tests/sketch_test_fixture.cpp`
- `tests/sketch_tests.cpp`, `tests/sketch_topo_tests.cpp`, `tests/sketch_ops_tests.cpp`, `tests/sketch_json_tests.cpp`
- `agents/drafts/issues/active/gh-189-extrude-sketch-face-test.md`

### Related

- Landed on unit-test refactor PR: https://github.com/trailcode/EzyCad/pull/188
- Broader suite split: https://github.com/trailcode/EzyCad/issues/186
- GitHub issue: https://github.com/trailcode/EzyCad/issues/189

### Test plan

- [x] Build `EzyCad_tests` (Debug).
- [x] `EzyCad_tests --gtest_filter=Sketch_test.ExtrudeSketchFace_EzyCad`
- [x] Comment cleanup only (no behavior change) for orphaned banners.
