---
github_issue: 215
status: active
paired_draft: gh-214-shape-list-hierarchy-phase3.md
---

# Shape List hierarchy phase 2

**Suggested labels:** `enhancement`, `ui`

---

## Title (GitHub)

Shape List hierarchy phase 2: reorder, duplicate group, selection polish

## Body (GitHub)

### Summary

Follow-up work after the organizational Shape List outliner (phase 1). Phase 1 already covers groups, current group, DnD reparent, cascade delete/ungroup, visibility, boolean result parenting, and STEP XCAF assembly import as groups. Phase 2 hardens outliner workflows that still feel incomplete without changing leaf BREP or adding parent transforms.

### Context (phase 1 done)

- Hierarchical Shape List with organizational groups (`parentId` / `order` / `isGroup`)
- Current group; new primitives/extrudes/revolves land under it
- Group / New group / Ungroup, DnD reparent, cascade delete, Ctrl multi-select
- STEP import preserves XCAF product/assembly groups (unless **Union shapes**)

Related: #59 (broader compound/assembly modeling; this issue is the next Shape List slice).

### Scope

**In scope**

- **Reorder without reparent** — drag between siblings (or explicit move up/down) to change `sibling_order` under the same parent without changing `parent_id`.
- **Duplicate group** — deep-copy a group subtree (structure + leaf solids), unique names, undoable.
- **Selection / hover polish** — e.g. Select children only, clearer empty-group vs solid selection feedback, optional middle-click or context-menu variants for subtree selection.
- Optional stretch: scripting helpers for `parent_id` / children / `group()` / `ungroup()` if low cost.

**Out of scope (phase 3)**

- Parent transform inheritance (moving a group moves children)
- Boolean feature-history trees (fuse keeps inputs as children)
- Part-local origins / planes
- Cross-sketch parenting

### Acceptance criteria

- [ ] User can reorder siblings under the same parent without reparenting to another group
- [ ] Duplicate group creates an independent deep copy (structure + solids) with undo
- [ ] At least one polished subtree-selection path beyond click-group selects all descendant solids
- [ ] Docs (`docs/usage.md`, `src/doc/gui.md` / `shape.md`) and CHANGELOG updated
- [ ] Unit tests for reorder and duplicate-group (plus any selection helpers that are non-UI)

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/215
- #59 Compound / assembly modeling in EzyCad
- #214 Shape List hierarchy phase 3
