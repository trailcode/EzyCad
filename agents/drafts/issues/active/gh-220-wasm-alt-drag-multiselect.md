---
github_issue: 220
status: active
---

# WASM: Alt + LMB drag does not multi-select shapes

**Suggested labels:** `bug`

**Opened:** https://github.com/trailcode/EzyCad/issues/220

---

## Title (GitHub)

WASM: Alt + LMB drag does not multi-select shapes

## Body (GitHub)

See https://github.com/trailcode/EzyCad/issues/220

### Summary

Desktop: **Alt + LMB drag** rubber-band multi-selects shapes via OCCT `AIS_ViewController`. WASM: same gesture did not select multiple shapes because live Alt was never polled (`m_occt_window` null on Emscripten).

### Fix (working tree)

- `Occt_view` stores non-owning `GLFWwindow* m_glfw_window` for modifier/cursor polling.
- Docs: `usage.md`, `usage-occt-view.md`, `gui.md`, CHANGELOG, plan `wasm-alt-drag-multiselect.md`.

### Acceptance criteria

- [x] Code path: WASM can poll Alt during drag (same as desktop)
- [ ] Manual WASM: Alt + LMB drag multi-selects like desktop
- [x] Desktop gesture map unchanged
- [x] Docs note Alt+drag (and Web Alt caveat)
- [x] Plan updated: `agents/plans/wasm-alt-drag-multiselect.md`

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/220
- Plan: `agents/plans/wasm-alt-drag-multiselect.md`
- Related: #93 / `gh-93-emscripten-web-hotkeys-followup.md`
