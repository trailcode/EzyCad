---
github_issue: 218
github_pr: 219
status: active
paired_draft: ../prs/active/gh-219-cross-section-tool.md
---

# Add temporary shape cross-section preview tool

**Suggested labels:** `enhancement`, `ui`, `feature-request`

---

## Title (GitHub)

Add temporary shape cross-section preview tool

## Body (GitHub)

See https://github.com/trailcode/EzyCad/issues/218

### Summary

Interactive **Shape cross-section** tool: shared local plane cut of selected solids with temporary AIS preview (no sketch commit in v0).

### Acceptance criteria

- [ ] Auto-preview on enter / selection / plane / offset
- [ ] Bold empty-selection prompt; leave mode or clear selection removes preview
- [ ] Focused tests `Shp_cross_section.*` / `Shp_test.Cross_section*`
- [ ] Docs + CHANGELOG updated

### Related

- PR: https://github.com/trailcode/EzyCad/pull/219
- Plan: `agents/plans/cross-section-tool.md`
