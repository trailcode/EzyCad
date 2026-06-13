# PR - [branch-name or short title]

## Title

[Concise PR title for GitHub]

## Summary

- Bullet list of high-level changes.
- Why the change was needed (ties back to issue if any).
- Any notable design decisions or follow-ups.

## Files Changed

- `src/foo.cpp`
- `src/foo.h`
- `docs/usage-sketch.md`
- `CHANGELOG.md`
- `agents/issues/NNN-....md` (or `agents/prs/NNN-....md`)
- ...

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/NNN
- Compare / branch link (once pushed).
- Other drafts: `agents/issues/XXX-....md`

## Test Plan

- [ ] Build `EzyCad_lib` (Debug + Release) and `EzyCad_tests`.
- [ ] Run relevant tests (`EzyCad_tests --gtest_filter=...` or full via CTest).
- [ ] Manual verification steps for the feature / fix.
- [ ] Docs build (`sphinx-build -b html -W docs docs/_build`) and review rendered pages.
- [ ] Check in-app behavior (Options pane, hotkeys, tooltips, etc.).
- [ ] Verify no regressions in related areas (e.g., snap, material, view).
- [ ] (If user-visible) cross-check `agents/user-docs-sync.md`.

## Notes

- Any caveats, follow-up issues/PRs, or context for reviewers.
- Unrelated files or workspace state.
- CI / platform specifics (Windows native, Emscripten, etc.).

## Post-merge (checklist for the author)

- [ ] Update any open issue drafts or related PRs.
- [ ] Close linked GitHub issue(s) if fully addressed.
- [ ] Consider adding to `CHANGELOG.md` `[Unreleased]` if not already done.