---
github_issue: 214
status: active
paired_draft: gh-215-shape-list-hierarchy-phase2.md
---

# Shape List hierarchy phase 3

**Suggested labels:** `enhancement`, `feature-request`

---

## Title (GitHub)

Shape List hierarchy phase 3: parent transforms and advanced assembly model

## Body (GitHub)

### Summary

Longer-term Shape List / document hierarchy work that should **not** ship with the organizational outliner. Phase 1 delivered folder-style groups without changing leaf BREP or transform semantics. Phase 2 covers outliner polish (reorder, duplicate, selection). Phase 3 is the product decision layer: inherited placement, optional boolean history, and part-local frames.

### Context

- Phase 1: organizational groups only — parents are folders; leaf solids remain the only move/boolean targets.
- Phase 2: #215 reorder, duplicate group, selection polish.
- Related: #59 (compound/assembly modeling; XCAF/instance transforms overlap this phase).

### Scope (candidates — pick deliberately)

**Parent transform inheritance**

- Move / rotate / scale a group applies to descendants (relative placement).
- Needs clear interaction with `Shp_move` / `Shp_rotate` / `Shp_scale`, undo, and baking vs live local transforms.

**Boolean feature history tree**

- Fuse/cut/common keep inputs as children instead of today's replace-input model.
- Separate product decision; conflicts with current boolean UX and deltas.

**Part-local origins / planes**

- FreeCAD-like coordinate objects under a part/group — new object types, not just grouping.

**Cross-sketch parenting**

- Explicitly **out of product intent** for now: sketches stay in Sketch List.

### Acceptance criteria (once scoped)

- [ ] Written design note choosing which of the candidates ship (and which stay deferred)
- [ ] Implementation matches that design; leaf-only ops remain safe where inheritance does not apply
- [ ] Undo/redo covers new transform or history semantics
- [ ] User docs and CHANGELOG updated
- [ ] Tests for the chosen model (placement inheritance and/or history)

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/214
- #59 Compound / assembly modeling in EzyCad
- #215 Shape List hierarchy phase 2
