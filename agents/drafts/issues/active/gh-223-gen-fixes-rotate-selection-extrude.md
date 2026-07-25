---
github_issue: 223
github_pr: 224
status: active
paired_draft: ../prs/active/gh-224-gen-fixes-rotate-selection-extrude.md
---

# Fix shape rotate Tab degrees, selection restore, and extrude finalize click

**Suggested labels:** `bug`, `ui`

---

## Title (GitHub)

Fix shape rotate Tab degrees, selection restore, and extrude finalize click

## Body (GitHub)

### Summary

Shape transform and extrude polish: Rotate Tab angle entry showed length units and Enter did not commit when a unit suffix was shown; entering Move/Rotate/Scale dropped pre-selected solids; confirming an active sketch extrude with LMB could re-pick the face on the same click.

### Problem

- **Rotate Tab:** `Shp_rotate::show_angle_edit` opened `set_dist_edit` (`in`/`mm`) while treating the value as degrees. Docs already say degrees.
- **Dist/angle Enter:** After project-unit suffixes (`in`/`mm`) and the new `deg` label, `IsItemDeactivatedAfterEdit()` was queried after the suffix `Text` widget, so Enter / click-away never committed.
- **Pre-selection:** Entering Move / Rotate / Scale wiped AIS selection during selection-mode / faint redisplay (cross-section already snapped selection; transforms did not).
- **Extrude finalize:** LMB that finalized an active extrude still ran `on_left_click_`, which could start another face pick on the same press.

### Implemented scope

**Code changes:**

- `src/shp_rotate.cpp`: Tab angle entry uses `set_angle_edit`.
- `src/gui.cpp`: snapshot `IsItemDeactivatedAfterEdit` before unit/`deg` suffix; skip `on_left_click_` when extrude finalize consumed LMB; angle popup shows `deg`.
- `src/gui_mode.cpp`: Move/Rotate skip sketch Tab `dimension_input` / `angle_input`.
- `src/gui_occt_view.cpp` / `.h`: preserve selection on Move/Rotate/Scale/cross-section enter; call `m_shp_extrude` directly (drop thin wrappers).
- `src/shp_extrude.cpp`: remove debug noise on finalize.

**Documentation:**

- `CHANGELOG.md` Fixed entries for Rotate Tab and dist/angle Enter.
- `src/doc/gui.md`, `src/doc/shape.md` updated.
- User guide (`docs/usage.md`) already correct for Rotate Tab; no usage change.

### Acceptance criteria

- [ ] Rotate: select shape(s) → R → Tab → type degrees → Enter commits and finalizes; popup shows `deg`.
- [ ] Move Tab distance (and other dist/angle popups with a unit suffix): Enter / click-away commit.
- [ ] Select solids → G / R / S: selection remains; first mouse move uses those shapes.
- [ ] Active extrude preview → LMB confirms once without immediately re-picking a face.
- [ ] Dev docs / CHANGELOG match behavior.

### Files touched

- `src/shp_rotate.cpp`
- `src/gui.cpp`
- `src/gui_mode.cpp`
- `src/gui_occt_view.cpp`
- `src/gui_occt_view.h`
- `src/shp_extrude.cpp`
- `src/doc/gui.md`
- `src/doc/shape.md`
- `CHANGELOG.md`
- `agents/drafts/issues/active/gh-223-gen-fixes-rotate-selection-extrude.md` (this draft)
- `agents/drafts/prs/active/gh-224-gen-fixes-rotate-selection-extrude.md`

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/223
- PR: https://github.com/trailcode/EzyCad/pull/224
- Branch: `Trailcode/gen-fixes`

### Test plan

- [ ] Desktop build Debug/Release.
- [ ] Manual: Rotate Tab degrees + Enter; Move Tab distance + Enter.
- [ ] Manual: pre-select solids, enter G/R/S, confirm selection kept.
- [ ] Manual: extrude face, LMB to confirm once.
- [ ] Spot-check `docs/usage.md` Rotate section still accurate.
