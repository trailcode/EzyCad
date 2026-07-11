---
github_pr: 188
github_issue: 186
status: active
paired_draft: ../issues/active/gh-186-unit-test-refactor.md
---

# PR - Split sketch unit tests; move tools/ into scripts/

## Title

Split sketch unit tests and move tools/ helpers into scripts/

## Summary

- Extract shared `Sketch_test` fixture and split `sketch_tests.cpp` into topo / ops / JSON / core suites (same `Sketch_test.*` filter).
- Bridge regression dumps use Geometry Watch `ezy_geom` polygons (`dbg_faces`), not console output.
- Move `tools/` helpers into `scripts/` and drop the empty `tools/` tree / CMake glob.
- Enable `ExtrudeSketchFace_EzyCad` via `Shp_extrude_access`; remove public test hooks from `Shp_extrude`.
- Clean orphaned / duplicate / stale comments left after the suite split.

## Files Changed

- `tests/sketch_test_fixture.*`, `tests/sketch_*_tests.cpp`, `tests/sketch_tests.cpp`
- `src/shp_extrude.h`, `src/shp_extrude.cpp`
- `scripts/pimpl_gen.py`, `scripts/pbf-to-png.*`, `scripts/fix_doc_img_tags.py`, `scripts/ezycad_graphical_debugging.xml`
- `CMakeLists.txt`, `.gitignore`, `.gitattributes`
- `src/doc/sketch.md`, `gui.md`, `utility.md`, `agents/conventions/token-lean.md`, `agents/workflows/local-dev.md`
- `agents/drafts/issues/active/gh-189-extrude-sketch-face-test.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/186
- Issue: https://github.com/trailcode/EzyCad/issues/189
- Branch: `Trailcode/unit-test-refactor`
- Pages size follow-up: https://github.com/trailcode/EzyCad/issues/187

## Test Plan

- [x] Build `EzyCad_tests` (Debug/Release)
- [x] `EzyCad_tests --gtest_filter=Sketch_test.ExtrudeSketchFace_EzyCad`
- [x] `EzyCad_tests --gtest_filter=Sketch_test.*`
- [ ] CI green on the PR

## Post-merge

- [ ] Archive issue/PR drafts (186, 188, 189)
- [ ] Consider `CHANGELOG.md` `[Unreleased]` if desired (internal refactor)
