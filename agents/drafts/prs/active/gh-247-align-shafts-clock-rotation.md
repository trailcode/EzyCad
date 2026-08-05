---
github_issue: 246
github_pr: 247
status: open
---

# PR - Align shafts: clock rotation and rename

## Title

Align shafts: clock rotation and rename

## Summary

- Rename Align cylinders -> Align shafts (user-facing; `mode.cyl_align` unchanged).
- Options Clock rotation (default off) for roll about shared axis after depth.
- Docs + CHANGELOG.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/246
- PR: https://github.com/trailcode/EzyCad/pull/247
- Branch: `Trailcode/allign_shafts`

## Test Plan

- [ ] Build Release; exercise Clock rotation off/on, Flip, Tab/Shift+Tab, rename in UI/docs.
