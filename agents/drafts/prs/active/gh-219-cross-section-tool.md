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

- Temporary shape cross-section preview tool (local XY/XZ/YZ + offset).
- Shape local frames for plane orientation / persistence.
- Auto-update preview from selection, plane, and offset; Options empty-selection prompt.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/218
- PR: https://github.com/trailcode/EzyCad/pull/219
- Branch: `Trailcode/cross-section`
- Plan: `agents/plans/cross-section-tool.md`

## Test Plan

- [ ] Desktop Release build
- [ ] `EzyCad_tests --gtest_filter=Shp_cross_section.*:Shp_test.Cross_section*`
- [ ] Manual enter / selection / plane / offset / Esc
- [ ] Docs sync checklist
