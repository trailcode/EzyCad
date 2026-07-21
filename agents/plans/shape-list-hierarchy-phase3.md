---
status: planning
topic: shape-list-hierarchy-phase3
depends_on: null
blocks: null
github_issue: 214
---

# Shape List hierarchy phase 3: placement, Parts, Boolean history

**Load only when** the prompt is about parent transform inheritance, Part/Origin/plane objects, editable Boolean history, or Shape List hierarchy phase 3 (#214). Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

Post-v1 work. Do **not** ship any of this in v1.

## Scope and fixed decisions

- Tracked by [gh-214 draft](../drafts/issues/active/gh-214-shape-list-hierarchy-phase3.md); builds on phase 1 groups and phase 2 (#215) polish.
- Add true relative placement: `world(node) = world(parent) * local(node)`; committed move/rotate/scale updates placement instead of baking BREP.
- Add typed Shape List nodes: Body, Group, Part, Boolean, Origin, and Plane. Group and Part are transform containers; Part owns locked Origin, XY, XZ, and YZ children.
- Implement Boolean nodes as **editable parametric features** with ordered operands, cached result BREP, dirty/error state, and bottom-up recomputation (user decision; replaces the destructive replace-input model).
- Keep sketches in `Occt_view::m_sketches` and the existing Sketch List. Part planes may later seed a copied world `gp_Pln`, but no sketch gets a Shape List parent or live attachment in this phase.

## 1. Establish the typed node and placement foundation

- Extend [`src/shp.h`](../../src/shp.h) / [`src/shp.cpp`](../../src/shp.cpp) with `Shape_node_kind`, relative `gp_Trsf` placement, system-node flags, and kind-specific payload. Body geometry and Boolean cached geometry remain node-local.
- Add hierarchy helpers (prefer a focused `src/shp_hierarchy.*`) for world placement, preserve-world reparenting, cycle/type validation, subtree traversal, and canonical transform roots that suppress selected descendants.
- Update [`src/gui_occt_view.h`](../../src/gui_occt_view.h) / [`src/gui_occt_view.cpp`](../../src/gui_occt_view.cpp) so display, selection, geometry consumers, and export explicitly materialize local or world BREP as needed.
- Persist a validated 3x4 placement matrix and node kind in `.ezy` format v4. Load v3 Group/Body entries with identity leaf placement and synthesized container frames while preserving visible world geometry; reject malformed transforms, cycles, missing parents, and invalid child kinds.
- Replace `current_group_id` with a current-container concept accepting root, Group, or Part.

## 2. Convert move, rotate, scale, and tree edits

- Refactor [`src/shp_operation.cpp`](../../src/shp_operation.cpp), [`src/shp_move.cpp`](../../src/shp_move.cpp), [`src/shp_rotate.cpp`](../../src/shp_rotate.cpp), and [`src/shp_scale.cpp`](../../src/shp_scale.cpp): previews remain presentation-only; finalize computes `newLocal = inverse(parentWorld) * deltaWorld * oldWorld`.
- Use logical Shape List selection for Group/Part/Boolean transform targets and viewer selection for bodies; canonicalize roots so an ancestor and descendant cannot transform twice.
- Define pivots: Part Origin for Part, stored origin for Group/Boolean, bbox center for one Body, and combined world bbox for multi-selection. Retain positive uniform scale and the existing clamp (0.01..100).
- Make drag/drop, grouping, ungrouping, and dissolve preserve world pose with `newLocal = inverse(newParentWorld) * oldWorld`.
- Extend [`src/shp_delta.h`](../../src/shp_delta.h) / [`src/shp_delta.cpp`](../../src/shp_delta.cpp) with placement deltas and placement-aware tree links. Undo/redo restores recorded placements and cached feature state without rerunning OCCT.

## 3. Add Part, Origin, and plane objects

- Add New Part and Part-aware insertion to [`src/gui.cpp`](../../src/gui.cpp). Creating a Part atomically creates one locked Origin and three locked standard Plane nodes with stable IDs and fixed ordering.
- Prevent independent delete, duplicate, reorder, or reparent of system Origin/Plane nodes; deleting or dissolving a Part handles them as one typed undo transaction.
- Add a small viewer presentation helper for origin/plane annotations. Their pose inherits Part placement, while glyph size remains screen-relative and ignores inherited scale.
- Supersedes the annotation-only frame lean in [shp-origin-orientation.md](shp-origin-orientation.md): the Part Origin is the actual placement frame; Body frame UX can build on the same model.

## 4. Implement editable parametric Boolean history

- Add a Boolean evaluator (prefer `src/shp_boolean_feature.*`) holding operation kind, authoritative ordered operand IDs, cached local result, and Clean/Dirty/Error state.
- Refactor [`src/shp_fuse.cpp`](../../src/shp_fuse.cpp), [`src/shp_cut.cpp`](../../src/shp_cut.cpp), and [`src/shp_common.cpp`](../../src/shp_common.cpp) to create a Boolean node, preserve-world reparent operands beneath it, evaluate in Boolean-local coordinates, and commit atomically only after initial success.
- Recompute affected Boolean ancestors bottom-up after operand geometry, relative placement, operation, or order changes. Moving an entire Boolean or its ancestor does not dirty it.
- On later recompute failure, retain the last-good cached result, expose operands, mark Error, and allow recovery. Cut keeps operand 0 as Base; Fuse/Common retain explicit order for deterministic persistence.
- Add typed feature/Boolean deltas for create, dissolve, operation/order edits, operand removal, and cached results. Generic drag/drop must not mutate Boolean operand membership.

## 5. Complete lifecycle, UI, and interchange semantics

- Expand Boolean rows as Base/Tool or numbered Operand children; add operation change, reorder, recompute, show operands, remove operand, and dissolve actions in [`src/gui.cpp`](../../src/gui.cpp).
- Cascade-delete Group, Part, and Boolean subtrees. Dissolve promotes ordinary children preserve-world; Part drops system references; Boolean removes its result and promotes operands.
- Traverse logical outputs for export: Body exports world geometry, Boolean exports its cached result once, Group/Part recurse, and Origin/Plane export nothing. Keep STEP/IGES/STL/PLY flattened; only `.ezy` preserves history.
- Keep STEP assemblies as Groups unless reliable metadata justifies Part. Keep "Union shapes" import as a one-time Body unless the command is explicitly changed to editable Fuse.

## 6. Verify and document each landing slice

- Add focused tests (split from [`tests/shp_tests.cpp`](../../tests/shp_tests.cpp) if useful) for v3-to-v4 migration, placement composition, preserve-world reparenting under rotated/scaled parents, no double-transform, and placement undo/redo.
- Test atomic Part system nodes, persistence, locked lifecycle, plane inheritance, and subtree undo.
- Test Fuse/Cut/Common ordered operands, nested recomputation, failure with last-good cache, recovery, dissolve/delete, and undo that does not invoke recomputation.
- Test export applies nested placement exactly once and excludes operands/reference nodes; smoke-test hierarchy selection and Boolean error UI.
- Update [`src/doc/shape.md`](../../src/doc/shape.md), [`src/doc/gui.md`](../../src/doc/gui.md), [`src/doc/undo-redo.md`](../../src/doc/undo-redo.md), [`docs/usage.md`](../../docs/usage.md), and `CHANGELOG.md` as each slice ships.

## Delivery gates

1. Placement/migration/reparent/export tests must pass before enabling inherited transforms.
2. Inherited transforms and Part references must be stable before Boolean history depends on them.
3. Boolean history ships only after nested recompute, failure recovery, persistence, undo, and export tests pass.
4. No phase 3 behavior is included in v1; sketches remain structurally unchanged throughout.
