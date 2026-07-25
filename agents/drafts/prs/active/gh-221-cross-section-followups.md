---
github_issue: 221
github_pr: 222
status: active
paired_draft: ../issues/active/gh-221-cross-section-followups.md
---

# PR - Trailcode/cross-section-improve

## Title

Cross-section follow-ups: outline, Clip discard, sketch import, toast log

## Summary

- Optional **Show section outline** (default off); status toasts also write to the Log.
- **Clip** removes fully discarded solids; keeps clipped survivors (undoable).
- **Cross section sketch** imports section line/circle edges into a new sketch (undoable).
- Docs, CHANGELOG, and focused tests.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/221
- PR: https://github.com/trailcode/EzyCad/pull/222
- Branch: `Trailcode/cross-section-improve`
- Prior: #218 / #219

## Test Plan

- [ ] Desktop Release build
- [ ] `EzyCad_tests --gtest_filter=Shp_cross_section.*:Shp_test.Cross_section*`
- [ ] Manual outline / Clip discard / Cross section sketch / toast→Log
- [ ] Docs sync checklist
