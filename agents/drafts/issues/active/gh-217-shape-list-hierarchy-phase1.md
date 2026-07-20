---
github_issue: 217
github_pr: 216
status: active
paired_draft: ../../prs/active/gh-216-shape-list-hierarchy-phase1.md
---

# Shape List hierarchy phase 1

**Suggested labels:** `enhancement`, `ui`

---

## Title (GitHub)

Shape List hierarchy phase 1: organizational groups and outliner

## Body (GitHub)

### Summary

Documents the Shape List organizational hierarchy and related import/outliner work landed in PR #216 (phase 1). Groups are folder nodes in the document tree (no parent transform inheritance). Follow-ups: #215 (phase 2), #214 (phase 3), #59 (broader assembly modeling).

### Problem

- Shape List was a flat list of solids with no parent/child outliner.
- Multi-body STEP assemblies flattened; no way to organize parts into folders for visibility, selection, or parenting new geometry.
- Hide-all and selection UX did not support group-oriented workflows.

### Implemented scope

**Document model**

- `Shp` nodes gain `parent_id`, `sibling_order`, `is_group` (empty compound; never displayed).
- `.ezy` persists `parentId`, `order`, `isGroup`, `visible`.
- Undo: `Shape_tree_delta` for reparent / group / ungroup; shape add/remove records include hierarchy fields.

**Shape List UI**

- Tree table: expand/collapse, vis / solid-wire / material on the left, name on the right.
- **Current group**: click a group (including empty) to set it; new primitives, extrudes, revolves, PLY, and unioned STEP import parent under it.
- **New group** / **Group** / **Ungroup** (all direct children move up); cascade delete of group subtrees.
- Drag-drop reparent (onto group, onto solid → that solid's parent, empty pad below list → document root).
- Ctrl multi-select; group click selects descendant solids.
- Persist `ui.shapeList.expanded` and `ui.shapeList.currentGroupId`.

**Operations / import**

- Booleans / polar dup: result parent = shared operand parent, else root.
- Hide all no longer stomps per-shape visibility flags (effective visibility via ancestors + overlays).
- STEP XCAF assembly → Shape List group tree (unless **Union shapes**); product/part names used when present.
- File → Import dialog (metadata, optional union) — related import UX on the same branch.

**Tests / docs**

- `tests/shp_tests.cpp`: group/ungroup, hierarchy JSON, fuse parent, current-group parenting, etc.
- `docs/usage.md`, `CHANGELOG.md`, `src/doc/shape.md`, `gui.md`, `utility.md`.

### Acceptance criteria

- [x] Group / ungroup / reparent / cascade delete with undo
- [x] Current group; empty groups selectable; new solids parent under it
- [x] DnD to group, to sibling-via-solid, and to document root (empty pad)
- [x] Group visibility hides subtree; Hide all preserves per-node flags
- [x] Boolean results inherit shared parent when applicable
- [x] STEP assembly import as group tree (optional union)
- [x] `.ezy` round-trip of hierarchy + UI expand/current group
- [x] Unit tests and user/developer docs updated

### Out of scope (follow-ups)

- #215 — sibling reorder without reparent, duplicate group, selection polish
- #214 — parent transform inheritance, boolean history tree, part-local frames
- #59 — broader compound / assembly modeling

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/217
- PR: https://github.com/trailcode/EzyCad/pull/216
- #215 Shape List hierarchy phase 2
- #214 Shape List hierarchy phase 3
- #59 Compound / assembly modeling in EzyCad
