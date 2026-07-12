---
github_issue: 198
github_pr: 199
status: active
paired_draft: ../issues/active/gh-198-scripting-fuse-cut-common-delete.md
---

# PR - `Trailcode/remote-scripting`

## Title

Remote Python --listen, ezycad client, and shape boolean scripting

## Summary

- **Remote Python (`--listen`):** TCP listener with length-prefixed JSON; execution on the main UI thread; quit/hang fixes for open clients. Closes #118.
- **Public API:** `ezy` / `ezy.view` / `ezy.view.curr_sketch` (same as `ezy.sketch`); Lua mirrors; `add_box` / `add_sphere` return `Shp`.
- **Client package:** importable `scripts/ezycad` (`ezycad.connect()`) plus thin `ezycad_remote` CLI; `pyproject.toml` for editable install.
- **Booleans / delete:** `ezy.view.fuse` / `cut` / `common` / `delete` with shared C++ APIs used by GUI selection commands. Closes #198.
- **Docs:** `docs/scripting.md`, `src/doc/script.md`, `CHANGELOG.md`, local-dev notes.

## Files Changed

Remote / console / main, `shp_fuse` / `shp_cut` / `shp_common`, `scripts/ezycad/*`, `pyproject.toml`, scripting docs, CHANGELOG, `.gitignore` (stop tracking `*.egg-info/`).

## Related

- Issues: https://github.com/trailcode/EzyCad/issues/118 , https://github.com/trailcode/EzyCad/issues/198
- PR: https://github.com/trailcode/EzyCad/pull/199
- Supersedes empty tip on PR #119 (`feature/remote-python-server`)

## Test Plan

- [ ] Build Debug `EzyCad` with Python enabled
- [ ] `EzyCad --listen 8765`; `import ezycad; app = ezycad.connect()`; `app.view.add_box(...)` / `fuse` / `cut` / `common` / `delete`
- [ ] In-app Python/Lua: same `ezy.view.*` calls
- [ ] GUI Shape fuse/cut/common still work from multi-selection
- [ ] Quit with open remote clients does not hang
- [ ] Review `docs/scripting.md`
