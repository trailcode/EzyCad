---
github_issue: 196
github_pr: 197
status: active
paired_draft: ../prs/active/gh-197-occt-dual-version-handle-warnings.md
---

# OCCT dual-version (desktop 8 / WASM 7.9.3) harden

**Suggested labels:** `bug`, `documentation`, `refactor`

---

## Title (GitHub)

OCCT dual-version (desktop 8 / WASM 7.9.3): Handle aliases, failure messages, build warning cleanup

## Body (GitHub)

Filed: https://github.com/trailcode/EzyCad/issues/196

### Summary

Shared `src/` must build against desktop OCCT **8.0.0** and recommended WASM OCCT **7.9.3** until OCCT 8 GLES shading works for wasm. Harden that split: stop using `Handle(T)`, add dual-version `Standard_Failure` messaging, document agent conventions, clear native/WASM warnings.

### Acceptance criteria

- [x] `standard_failure_message()` used at `Standard_Failure` catch sites
- [x] `Handle(...)` -> `*_ptr` aliases in `utl_types.h`
- [x] Agent conventions wired (`occt-wasm-dual-version.md`, `occt-handles.md`)
- [x] Native/WASM listed warnings cleared
- [ ] CI green

### Related

- PR: https://github.com/trailcode/EzyCad/pull/197
- Draft: `agents/drafts/prs/active/gh-197-occt-dual-version-handle-warnings.md`
