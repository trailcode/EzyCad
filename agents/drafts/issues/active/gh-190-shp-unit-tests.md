---
github_issue: 190
github_pr: 191
status: active
paired_draft: ../prs/active/gh-191-shp-unit-tests.md
---

# Add dedicated shape unit tests (shp_tests) and document VS 2026 local builds

**Suggested labels:** `enhancement`, `docs`

---

## Title (GitHub)

Add dedicated shape unit tests (shp_tests) and document VS 2026 local builds

## Body (GitHub)

### Summary

Add a dedicated GTest suite for the shape module (`tests/shp_tests.cpp`) covering primitive creation, shape info, view registration, and boolean ops. Update developer docs so local Windows builds use Visual Studio 18 2026.

### Problem

- Shape behavior had no dedicated suite (`shp_tests` did not exist); coverage was only indirect via sketch/GUI tests and manual smoke.
- Local agent/dev docs still prescribed `Visual Studio 17 2022`, which fails on machines with only VS 2026 installed.

### Implemented scope

**Code changes:**

- `tests/shp_tests.cpp`: `Shp_create.*`, `Shp_info.*`, `Shp_test.*` (create volumes/bboxes, info lines, `Occt_view::add_*` / unique names, fuse/cut/common via AIS selection).

**Documentation:**

- `src/doc/shape.md` Testing section
- `agents/workflows/local-dev.md` and `README.md` Windows build notes -> VS 18 2026

### Acceptance criteria

- [x] `tests/shp_tests.cpp` covers create/info/view/booleans
- [x] `src/doc/shape.md` documents the suite
- [x] Local-dev / README use `Visual Studio 18 2026`
- [x] Filter `Shp_create.*:Shp_info.*:Shp_test.*` passes (16/16)
- [ ] CI green on the linked PR

### Files touched

- `tests/shp_tests.cpp`
- `src/doc/shape.md`
- `agents/workflows/local-dev.md`
- `README.md`
- `agents/drafts/issues/active/gh-190-shp-unit-tests.md` (this draft)

### Related

- PR: https://github.com/trailcode/EzyCad/pull/191
- Prior extrude test enablement: #189 / #188

### Test plan

- [x] Build `EzyCad_tests` (Debug) with `-G "Visual Studio 18 2026"`
- [x] `EzyCad_tests --gtest_filter=Shp_create.*:Shp_info.*:Shp_test.*`
