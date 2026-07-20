---
github_pr: 216
github_issue: 217
status: active
paired_draft: ../../issues/active/gh-217-shape-list-hierarchy-phase1.md
---

# PR - Shape List hierarchy phase 1

## Title

Shape List hierarchy phase 1: organizational groups and outliner

## Summary

- Organizational Shape List hierarchy (groups, current group, DnD including drop-to-root).
- Boolean/extrude parenting; Hide all preserves visibility flags.
- STEP XCAF assembly → group tree; Import dialog on this branch.
- Docs, CHANGELOG, tests.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/217
- PR: https://github.com/trailcode/EzyCad/pull/216
- Follow-ups: #215, #214, #59

## Test plan

- [ ] Shape List group / current group / DnD / ungroup / cascade delete
- [ ] Hide all / group visibility
- [ ] STEP import with/without Union shapes
- [ ] `EzyCad_tests.exe --gtest_filter=Shp_test.*`
