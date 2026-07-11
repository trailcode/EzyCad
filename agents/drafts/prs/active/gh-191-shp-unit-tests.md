---
github_pr: 191
github_issue: 190
status: active
paired_draft: ../issues/active/gh-190-shp-unit-tests.md
---

# PR - Shape unit tests and VS 2026 local build docs

## Title

Add shape unit tests and VS 2026 local build docs

## Summary

- Add `tests/shp_tests.cpp` with GTest coverage for `shp_create`, `shp_info`, view primitive registration / unique names, and fuse/cut/common (AIS selection injection).
- Document the suite in `src/doc/shape.md`.
- Point local Windows build docs at **Visual Studio 18 2026** (`agents/workflows/local-dev.md`, `README.md`); CI remains on VS 2022.

## Files Changed

- `tests/shp_tests.cpp`
- `src/doc/shape.md`
- `agents/workflows/local-dev.md`
- `README.md`
- `agents/drafts/issues/active/gh-190-shp-unit-tests.md`
- `agents/drafts/prs/active/gh-191-shp-unit-tests.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/190
- Branch: `Trailcode/shp-unit-tests`

## Test Plan

- [x] Build `EzyCad_tests` (Debug) with `-G "Visual Studio 18 2026"`
- [x] `EzyCad_tests --gtest_filter=Shp_create.*:Shp_info.*:Shp_test.*` (16/16)
- [ ] CI `windows-msvc` green

## Post-merge

- [ ] Archive issue/PR drafts
- [ ] Close #190 if fully addressed
