---
github_issue: 196
github_pr: 197
status: active
paired_draft: ../issues/active/gh-196-occt-dual-version-handle-warnings.md
---

# PR - OCCT dual-version harden

## Title

OCCT dual-version harden: Handle aliases, failure messages, warning cleanup

## Summary

- Desktop OCCT 8 vs WASM 7.9.3: `standard_failure_message()`, `*_ptr` over `Handle(T)`, agent conventions.
- Compiler warning cleanup (native + emscripten dock flags).

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/196
- PR: https://github.com/trailcode/EzyCad/pull/197
- Branch: `Trailcode/gen-fixes-refactor`

## Test Plan

- [x] Native Debug rebuild clean of listed warnings
- [x] WASM build-em-793 dock enum warning fixed
- [ ] CI Windows MSVC
- [ ] Spot-check OCCT error paths and Esc cancel

## Post-merge

- [ ] Archive this draft and paired issue draft
- [ ] Close #196 if fully addressed
- [ ] Consider `CHANGELOG.md` `[Unreleased]` note
