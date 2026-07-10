# Sketch face walk uses start-of-arc tangent as arrival direction

---
github_issue: 184
status: active
paired_draft: ../../prs/active/gh-185-sketch-face-walk-incoming-arc-tangent.md
---

**Suggested labels:** `bug`, `sketch`, `topology`

**Opened on GitHub:** https://github.com/trailcode/EzyCad/issues/184

## Title (GitHub)

Sketch face walk uses start-of-arc tangent as arrival direction (circle + chord drops faces)

## Body (GitHub)

### Summary

Face extraction in `Sketch_topo::update_faces_` treated `sketch_edge_outgoing_dir_2d` (tangent at the **start** of the traversed edge) as the arrival direction at the current node. On arcs those tangents differ, so the leftmost-turn rule can pick the wrong next edge and drop faces. Reproduced with a full circle split into arcs plus a chord (e.g. `project.ezy`: circle + chord `4->3`).

### Problem

- At chord endpoints the walker arrives on an arc; start and end tangents are not the same.
- Using the start tangent as `incoming_dir` mis-ranks candidate outgoing edges.
- Expected: two faces (small circular segment + large remainder). Wrong tangent can yield missing/incorrect faces.
- Straight edges were unaffected (chord direction is constant).

### Implemented scope

**Code changes:**

- `src/sketch_edge.h` / `src/sketch_edge.cpp` — add `sketch_edge_incoming_dir_2d`
- `src/sketch_topo.cpp` — use incoming dir for face-walk turn selection
- `tests/sketch_tests.cpp` — `UpdateFaces_CircleWithChord_IncomingArcTangent` (+ Geometry Watch debug vars)
- `tools/ezycad_graphical_debugging.xml`, `utl_geom*` — `to_boost_ls`, Ring registration for polygons

**Documentation:**

- `src/doc/sketch.md`, `src/doc/utility.md` — API / helper notes

### Acceptance criteria

- [x] Face walk uses arrival tangent at the current node for arcs (`sketch_edge_incoming_dir_2d`).
- [x] Regression test: circle of four arcs + chord produces two faces with correct areas.
- [x] `EzyCad_tests` filter `Sketch_test.UpdateFaces_CircleWithChord_IncomingArcTangent` passes.

### Files touched

- `src/sketch_edge.cpp`, `src/sketch_edge.h`, `src/sketch_topo.cpp`
- `src/utl_geom.cpp`, `src/utl_geom.h`, `src/utl_geom_boost.inl`
- `src/doc/sketch.md`, `src/doc/utility.md`
- `tests/sketch_tests.cpp`
- `tools/ezycad_graphical_debugging.xml`
- `agents/drafts/issues/active/gh-184-sketch-face-walk-incoming-arc-tangent.md` (this draft)

### Related

- PR: https://github.com/trailcode/EzyCad/pull/185
- Follow-up to #166; distinct from #165

### Test plan

- [x] `EzyCad_tests --gtest_filter=Sketch_test.UpdateFaces_CircleWithChord_IncomingArcTangent`
- [ ] Manual: circle + chord sketch shows both faces
