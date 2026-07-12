---
github_issue: 198
github_pr: 
status: active
paired_draft: ../prs/active/draft-remote-scripting-boolean-ops.md
---

# Scripting bindings for fuse, cut, common, and delete

**Suggested labels:** `enhancement`, `feature-request`

---

## Title (GitHub)

Scripting bindings for fuse, cut, common, and delete

## Body (GitHub)

See https://github.com/trailcode/EzyCad/issues/198

### Summary

Expose shape boolean operations and deletion through `ezy.view.*` for Python, Lua, and the remote `ezycad` client.

### Acceptance criteria

- [x] `Shp_fuse::fuse`, `Shp_cut::cut`, `Shp_common::common` take explicit `Shp` lists; GUI selection paths call them
- [x] Python / Lua `ezy.view.fuse` / `cut` / `common` / `delete`
- [x] Remote `scripts/ezycad` client mirrors methods
- [x] Docs + CHANGELOG
- [x] Debug build succeeds

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/198
- Remote listen: https://github.com/trailcode/EzyCad/issues/118
