---
github_issue: 186
github_pr: 188
status: active
paired_draft: ../prs/active/gh-188-unit-test-refactor.md
---

# Split sketch unit tests and consolidate tools into scripts/

**Suggested labels:** `refactor`, `tests`, `chore`

---

## Title (GitHub)

Split sketch unit tests and move `tools/` helpers into `scripts/`

## Body (GitHub)

### Summary

`tests/sketch_tests.cpp` had grown to ~3700 lines covering every sketch concern. Split it into focused suites aligned with the sketch modules, extract a shared fixture, and move repo helper scripts from `tools/` into `scripts/` so there is one top-level scripts folder.

### Problem

- Sketch GTest coverage lived in a single monolithic TU; hard to navigate and review.
- Production sketch code is already modular (`Sketch_topo`, operations, JSON); tests had not followed.
- `tools/` duplicated the role of `scripts/` for codegen and asset helpers.

### Implemented scope

**Code changes:**

- Shared fixture: `tests/sketch_test_fixture.h` / `.cpp` / `.inl` (`GUI_access`, `Sketch_access`, `View_access`, `Headless_guard`, `Sketch_test`).
- Suites: `sketch_tests.cpp` (core), `sketch_topo_tests.cpp`, `sketch_ops_tests.cpp`, `sketch_json_tests.cpp`.
- Bridge face dumps use Geometry Watch `ezy_geom` locals (`dbg_faces`) instead of console logging.
- Moved `pimpl_gen.py`, `pbf-to-png.*`, `fix_doc_img_tags.py`, `ezycad_graphical_debugging.xml` from `tools/` to `scripts/`; removed `tools/` CMake IDE glob.

**Documentation:**

- `src/doc/sketch.md`, `gui.md`, `utility.md`, `agents/conventions/token-lean.md`, `agents/workflows/local-dev.md` path updates.

### Acceptance criteria

- [x] `Sketch_test.*` still discovers and passes (74 cases).
- [x] Filter `EzyCad_tests --gtest_filter=Sketch_test.*` unchanged.
- [x] No `tools/` directory; helpers live under `scripts/`.
- [x] Native Release build of `EzyCad_tests` succeeds.

### Files touched

- `tests/sketch_*`
- `scripts/*` (moved from `tools/`)
- `CMakeLists.txt`, `.gitignore`, `.gitattributes`
- `src/doc/*`, `agents/conventions/token-lean.md`, `agents/workflows/local-dev.md`

### Related

- Follows sketch module split (#163 / #164).
- PR: https://github.com/trailcode/EzyCad/pull/188
- ExtrudeSketchFace enablement: #189
- GitHub Pages wasm demo size: #187
- Draft: `agents/drafts/issues/active/gh-186-unit-test-refactor.md`

### Test plan

- [x] Build `EzyCad_tests` (Release).
- [x] `EzyCad_tests --gtest_filter=Sketch_test.*`
