---
github_issue: 268
github_pr: 269
status: open
paired_draft: ../issues/active/gh-268-workbench-geometry-links.md
---

# PR - Workbench task with geometry links

## Title

Workbench task with geometry links

## Summary

- Sketch / Design / Workbench task toolbar. Workbench List stores geometry links with their own placement.
- Cut, fuse, and common reuse the first operand's `Shape_id`. Cross-section clip keeps each survivor's id. `Shape_replace_delta` does not drop links for an id the same delta inserts again.
- Extrude sketch face is on the Sketch task.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/268
- PR: https://github.com/trailcode/EzyCad/pull/269
- Branch: `Trailcode/workbench`

## Test Plan

- [x] `EzyCad_tests` cut / fuse / common / workbench link filters
- [ ] Two Workbench instances, cut the Design solid, undo and redo
- [ ] Delete the Design source and confirm instances disappear
