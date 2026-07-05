# PR - Sketch arc topology improvements

---
github_pr: 166
status: active
paired_draft: ../issues/active/gh-165-sketch-slot-circle-intersection-splitting.md
---

**Opened on GitHub:** https://github.com/trailcode/EzyCad/pull/166

Sketch arc topology: single edge per arc, intersection splitting, tangent face walker

## Summary

- Refactor arc sketch edges to **one graph edge per arc** (replacing two-edge internal representation).
- Split existing straight and arc edges when new arcs/lines cross them; subdivide new arcs at interior intersections.
- Face walking uses **arc tangents** at endpoints (fixes semicircle / slot cap loops).
- Arc Segment tool: rubber-band line after first click; **Add midpoint nodes** option adds geometric-mid snap node when enabled.
- Document arc behavior in `usage-sketch.md`; record open slot+circle bug in `bugs.md`.

## Files Changed

- `src/sketch_edge.cpp`, `src/sketch_edge.h`, `src/sketch_edges.cpp` — single arc edge, intersection split on add
- `src/sketch_topo.cpp`, `src/sketch_topo.h` — tangent-based face walker, arc/line split helpers, walk guard
- `src/utl_geom.cpp`, `src/utl_geom.h` — arc-line/arc-arc intersection helpers
- `src/sketch_tools.cpp`, `src/gui_mode.cpp` — arc preview, midpoint option in arc mode
- `src/sketch_delta.cpp`, `src/sketch_json.cpp`, `src/sketch_operations.cpp` — arc model migration
- `tests/sketch_tests.cpp` — arc split, semicircle walker, midpoint option tests
- `docs/usage-sketch.md`, `docs/bugs.md`, `CHANGELOG.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/165 (slot + circle bug — **not** fixed by this PR)
- Does **not** fix slot + circle intersection splitting; tracked separately.

## Test Plan

- [ ] Build `EzyCad_tests` (Debug).
- [ ] Run `Sketch_test.*Arc*`, `Sketch_test.*Semicircle*`, `Sketch_test.UpdateFaces_FaceWithArcs`.
- [ ] Manual: arc tool three-click flow, preview rubber band, optional midpoint node.
- [ ] Manual: line crossing arc splits both; slot + circle — confirm known bug still present (see issue).
- [ ] `sphinx-build -b html -W docs docs/_build`

## Notes

- `Walk_key` step cap in face walker prevents infinite loop on some parallel-edge cases; turn-rule improvement may follow.
- Circle finalize still uses legacy `add_arc_circle_(points[0], points[2], points[1])` call pattern; may need cleanup with slot+circle fix.
