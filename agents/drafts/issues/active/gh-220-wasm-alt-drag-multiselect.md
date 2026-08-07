---
github_issue: 220
github_pr: 250
status: active
paired_draft: ../prs/active/gh-250-wasm-alt-drag-multiselect.md
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

Desktop: **Alt + LMB drag** rubber-band multi-selects shapes via OCCT `AIS_ViewController`. WASM: same gesture did not multi-select because live Alt was never polled (`m_occt_window` null on Emscripten). Follow-up: stock `SelectRectangle` also missed some complex / STEP solids; Shape List current-group tint looked like selection.

### Fix (working tree)

- Non-owning `GLFWwindow* m_glfw_window` for modifier/cursor polling on WASM.
- After `SelectRectangle`, select displayed solids whose projected AABB intersects the band (GLFW/OCCT size mapping on WASM).
- Shape List: AIS selection tint vs weaker current-group tint.
- Docs: `usage.md`, `usage-occt-view.md`, `gui.md`, CHANGELOG, plan.

### Acceptance criteria

- [x] Code path: WASM can poll Alt during drag (same as desktop)
- [x] Manual WASM: Alt + LMB drag multi-selects like desktop (incl. complex solids under groups)
- [x] Desktop gesture map unchanged
- [x] Docs note Alt+drag (and Web Alt caveat)
- [x] Plan updated: `agents/plans/wasm-alt-drag-multiselect.md`
- [x] PR opened: https://github.com/trailcode/EzyCad/pull/250 (`Fixes #220`)
- [ ] Close GitHub #220 (on PR merge)

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/220
- PR: https://github.com/trailcode/EzyCad/pull/250
- Plan: `agents/plans/wasm-alt-drag-multiselect.md`
- Related: #93 / `gh-93-emscripten-web-hotkeys-followup.md`
