---
github_issue: 163
status: active
paired_draft: ../prs/active/gh-164-sketch-module-refactor.md
---

# Refactor monolithic Sketch into focused modules

**Suggested labels:** `refactor`, `sketch`, `enhancement`

**Branch:** `Trailcode/sketch_topo_refactor`

**GitHub:** https://github.com/trailcode/EzyCad/issues/163

---

## Title (GitHub)

Refactor monolithic Sketch into focused modules (edges, topo, dims, tools, node marks)

## Body (GitHub)

### Summary

Split the large `Sketch` class in `src/sketch.cpp` into smaller, single-responsibility modules so edge storage, planar topology, dimensions, interactive drawing tools, permanent node marks, AIS helpers, viewer display, and mirror/revolve operations each live in focused files. `Sketch` becomes a thin coordinator that owns sub-objects and delegates.

No user-visible behavior change is intended; this is structural cleanup following the `Sketch_underlay` extraction (#161 / PR #162).

### Problem

- `sketch.cpp` had grown into a god object (~3000 lines) mixing persistent geometry, face extraction, dimension UI, interactive tool state, JSON/delta paths, OCCT display helpers, and mirror/revolve logic.
- Hard to navigate, review, and test in isolation.
- Transient draw state (`m_tmp_edges`, previews, finalize/cancel) was intertwined with committed edge list and topology.

### Implemented scope

**New modules / translation units:**

| Module / file | Responsibility |
| --- | --- |
| `sketch_edge` | `Sketch_edge` type and `sketch_edge_is_linear()` |
| `Sketch_edges` | Persistent edge list; add/split/remove; JSON linear edges; read-only `for_each_linear` / `for_each_arc` visitors |
| `Sketch_topo` | Face extraction, `split_linear_edges_at_node_if_interior`, face caches |
| `Sketch_dims` | Length dimensions, typed input, pick state, tmp dimension preview |
| `Sketch_node_marks` | Permanent node '+' markers (`sync`, `remove_at`, etc.) |
| `sketch_ais` | `Sketch_AIS_edge`, `Sketch_AIS_node_mark`, `Sketch_face_shp`, mark removal helper |
| `Sketch_tools` | Interactive drawing session: tmp edges/nodes, mode-specific tools, finalize/cancel |
| `sketch_display.cpp` | Viewer display: visibility, edge styling, sketch switching, list hover (state on `Sketch`) |
| `sketch_operations.cpp` | Operation axis, mirror, revolve (state on `Sketch`) |

**`Sketch` layout after refactor:**

```
Sketch
  ├── Sketch_nodes
  ├── Sketch_node_marks
  ├── Sketch_edges
  ├── Sketch_topo
  ├── Sketch_dims
  ├── Sketch_tools
  └── Sketch_underlay
```

`sketch.cpp` (~400 lines) retains coordinator logic, input routing, edge shape updates, snap helpers, and inspector labels. Display and operations are split into dedicated `.cpp` files without new wrapper classes.

**Call-site updates:**

- `sketch_json.cpp`, `sketch_delta.cpp` use edge visitors and module delegates.
- `sketch_dims.cpp` reads tmp session state via `m_sketch.m_tools` accessors.
- `utl_types.h` includes `sketch_ais.h` for shared AIS typedefs.

**Follow-up (out of scope):**

- [ ] Deduplicate batch edge commit in `Sketch_tools::finalize_edges_` with `Sketch_edges::add_edge_impl_` (shared `commit_linear_batch` helper).
- [ ] Fix or retire `tools/gen_sketch_tools.py` generator script if kept.

### Acceptance criteria

- [x] `Sketch` delegates drawing to `Sketch_tools`; topology to `Sketch_topo`; edges to `Sketch_edges`.
- [x] Display and operations logic split into `sketch_display.cpp` / `sketch_operations.cpp`.
- [x] All `Sketch_test.*` cases pass (53 tests).
- [x] Native Release build succeeds (`EzyCad_lib`, `EzyCad_tests`).
- [x] No user-facing doc updates required (internal refactor only).

### Files touched

- `src/sketch.cpp`, `src/sketch.h` (slim coordinator)
- `src/sketch_edge.*`, `src/sketch_edges.*`
- `src/sketch_topo.*`, `src/sketch_dims.*`
- `src/sketch_tools.*`, `src/sketch_node_marks.*`, `src/sketch_ais.*`
- `src/sketch_display.cpp`, `src/sketch_operations.cpp`
- `src/sketch_json.cpp`, `src/sketch_delta.cpp`, `src/utl_types.h`
- `tests/sketch_tests.cpp`

### Test plan

- [x] Build `EzyCad_tests` (Release).
- [x] Run `EzyCad_tests --gtest_filter=Sketch_test.*` (53 cases).
- [ ] Manual smoke: add line, square, circle, arc; undo/redo; mirror/revolve with operation axis.
