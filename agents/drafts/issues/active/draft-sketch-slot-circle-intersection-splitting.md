# Sketch: slot + circle edge splitting at intersections

**Suggested labels:** `bug`, `sketch`

---

## Title (GitHub)

Sketch: slot + circle edges split or highlight incorrectly at intersections

## Body (GitHub)

### Summary

When a slot (two horizontal lines + end-cap arcs) is combined with a circle that crosses the horizontals, edge splitting and display can be wrong: one horizontal may split while the other does not, circle arcs may fail to split at line crossings, and some segments show incorrect highlight colors.

### Problem

- Adding end caps can spuriously split slot horizontals near the slot center (before any circle is added).
- Adding a circle that should cross both horizontals at two points each may not produce symmetric topology.
- Face update / edge styling can show inconsistent colors (e.g. white vs green segments).

### Likely causes (investigated, not yet fixed)

- `segment_arc_intersections_2d` (`utl_geom.cpp`): `GeomAPI_ExtremaCurveCurve` may return segment points paired with unrelated arc-foot points on the full circle outside the trimmed semicircle.
- `add_arc_circle_edges` / circle finalize argument order vs `GC_MakeArcOfCircle` node index conventions.
- Face walker parallel-edge loops (partial guard in `sketch_topo.cpp`).

### Acceptance criteria

- [ ] Slot horizontals remain unsplit after end caps are added (only endpoint nodes at corners).
- [ ] Circle crossing both horizontals splits each into three segments at the two intersection points.
- [ ] Circle arcs split at horizontal crossings; edge highlight colors consistent.
- [ ] `update_faces()` completes without infinite loop on slot + circle geometry.
- [ ] Regression tests: `SlotHorizontalLinesUnsplitBeforeCircleArcs`, `ArcSplit_CircleCrossesSlotLines`.

### Files / areas

- `src/utl_geom.cpp` — `segment_arc_intersections_2d`
- `src/sketch_edges.cpp` — `add_arc_circle_edges`
- `src/sketch_tools.cpp` — `finalize_circle_`, slot finalize
- `src/sketch_topo.cpp` — face walker
- `docs/bugs.md` — user-visible note
- `tests/sketch_tests.cpp`

### Related

- Documented in `docs/bugs.md`
- Arc topology PR (single edge per arc, intersection splitting) — separate work; does not resolve this bug

### Test plan

- [ ] Reproduce: slot tool + circle tool on same sketch, circle diameter larger than slot height.
- [ ] Verify edge counts per horizontal line before/after circle.
- [ ] Run targeted `EzyCad_tests` filters above.
