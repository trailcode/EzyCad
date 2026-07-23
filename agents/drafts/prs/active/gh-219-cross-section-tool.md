---
github_issue: 218
github_pr: 219
status: active
paired_draft: ../issues/active/gh-218-cross-section-tool.md
---

# PR - Trailcode/cross-section

## Title

Add temporary shape cross-section preview tool

## Summary

- Temporary shape cross-section preview (local XY/XZ/YZ + offset, Invert normal, Hide back side, Clip).
- Async section wires + desktop parallel pool; WASM serial (see `wasm-multithreading.md`).
- Shape local frames for plane orientation / persistence.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/218
- PR: https://github.com/trailcode/EzyCad/pull/219
- Branch: `Trailcode/cross-section`
- Plan: `agents/plans/cross-section-tool.md`

## Test Plan

- [ ] Desktop Release build
- [ ] `EzyCad_tests --gtest_filter=Shp_cross_section.*:Shp_test.Cross_section*`
- [ ] Manual enter / selection / plane / offset / Hide back / Clip / Esc
- [ ] Docs sync checklist
