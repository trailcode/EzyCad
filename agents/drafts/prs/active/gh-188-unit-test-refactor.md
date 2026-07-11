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

## Files Changed

- `tests/sketch_test_fixture.*`, `tests/sketch_*_tests.cpp`, `tests/sketch_tests.cpp`
- `scripts/pimpl_gen.py`, `scripts/pbf-to-png.*`, `scripts/fix_doc_img_tags.py`, `scripts/ezycad_graphical_debugging.xml`
- `CMakeLists.txt`, `.gitignore`, `.gitattributes`
- `src/doc/sketch.md`, `gui.md`, `utility.md`, `agents/conventions/token-lean.md`, `agents/workflows/local-dev.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/186
- Branch: `Trailcode/unit-test-refactor`
- Pages size follow-up: https://github.com/trailcode/EzyCad/issues/187

## Test Plan

- [x] Build `EzyCad_tests` (Release)
- [x] `EzyCad_tests --gtest_filter=Sketch_test.*` (74 passed)
- [ ] CI green on the PR

## Post-merge

- [ ] Archive issue/PR drafts
- [ ] Consider `CHANGELOG.md` `[Unreleased]` if desired (internal refactor)
