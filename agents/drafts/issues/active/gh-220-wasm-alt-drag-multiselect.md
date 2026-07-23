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

Desktop: **Alt + LMB drag** rubber-band multi-selects shapes via OCCT `AIS_ViewController`. WASM: same gesture does not select multiple shapes.

### Acceptance criteria

- [ ] WASM Alt + LMB drag multi-selects like desktop
- [ ] Desktop unchanged
- [ ] Docs note Alt+drag (and any Web caveats)
- [ ] Plan updated: `agents/plans/wasm-alt-drag-multiselect.md`

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/220
- Plan: `agents/plans/wasm-alt-drag-multiselect.md`
- Related: #93 / `gh-93-emscripten-web-hotkeys-followup.md`
