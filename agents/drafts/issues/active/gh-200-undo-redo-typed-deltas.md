---
github_issue: 200
github_pr: 201
status: active
paired_draft: ../prs/active/gh-201-undo-redo-typed-deltas.md
---

# Replace full-document JSON undo with typed shape/document deltas

**Suggested labels:** `enhancement`

---

## Title (GitHub)

Replace full-document JSON undo with typed shape/document deltas

## Body (GitHub)

### Summary

Interactive undo/redo still used full-document `to_json()` snapshots for almost every non-sketch edit. Extend the existing `Delta` pattern (already used for sketch geometry) to shapes and document structure so history stores only affected objects.

### Problem

- Full JSON undo copies every shape BREP on each step and reloads the whole model.
- Shapes lacked stable ids for delta resolution.

### Implemented scope

**Code changes:**

- Stable `Shape_id` on `Shp`; `shapes[].id` in project JSON.
- `src/shp_delta.*` — add / remove / geom / replace deltas (`TopoDS_Shape` in `Shape_rec`).
- `src/doc_delta.*` — sketch structural + underlay deltas.
- Wired `shp_*`, primitives, delete, extrude/revolve/import, sketch add/remove, underlay UI.
- JSON snapshots only for mixed delete and file-open checkpoint.

**Documentation:**

- `src/doc/undo-redo.md`, `shape.md`, `utility.md`

### Acceptance criteria

- [x] Interactive edits use typed deltas (no full-document copy).
- [x] Fuse/cut/delete/add-box undo tests; sketch delta interleave.
- [x] Shape ids persist in JSON.
- [x] Developer docs updated.

### Files touched

- `src/shp_delta.*`, `src/doc_delta.*`, `src/shp.*`, `src/gui_occt_view.*`, `src/gui.*`, `src/shp_*.cpp`
- `tests/shp_tests.cpp`
- `src/doc/undo-redo.md`, `shape.md`, `utility.md`
- `agents/drafts/issues/active/gh-200-undo-redo-typed-deltas.md` (this draft)

### Related

- GitHub issue: https://github.com/trailcode/EzyCad/issues/200
- Prior sketch deltas: #144 / #145
- PR: https://github.com/trailcode/EzyCad/pull/201
- PR draft: `agents/drafts/prs/active/gh-201-undo-redo-typed-deltas.md`

### Test plan

- [x] `EzyCad_tests --gtest_filter=Shp_test.Undo*:Shp_test.Shape_ids*:Sketch_test.Undo*`
- [ ] Manual smoke: box add, fuse, new sketch, underlay slider undo
