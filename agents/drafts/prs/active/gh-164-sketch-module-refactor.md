---
github_issue: 163
github_pr: 164
status: active
paired_draft: ../issues/active/gh-163-sketch-module-refactor.md
---

## Title

Refactor monolithic Sketch into focused modules (edges, topo, dims, tools, node marks)

## Summary

- Split `sketch.cpp` (~3000 lines) into focused modules: `Sketch_edges`, `Sketch_topo`, `Sketch_dims`, `Sketch_tools`, `Sketch_node_marks`, and `sketch_ais`.
- `Sketch` now owns sub-objects and delegates: interactive drawing to `Sketch_tools`, face/split logic to `Sketch_topo`, persistent edges to `Sketch_edges`, dimensions to `Sketch_dims`.
- Added read-only edge visitors (`for_each_linear` / `for_each_arc`) for JSON and delta paths.
- Moved AIS sketch types (`Sketch_AIS_edge`, `Sketch_AIS_node_mark`, `Sketch_face_shp`) into `sketch_ais.h`.
- Light polish: moved display and operations implementations to `sketch_display.cpp` and `sketch_operations.cpp` (no new wrapper classes; state stays on `Sketch`). `sketch.cpp` is ~400 lines.
- No intended user-visible behavior change; structural cleanup after `Sketch_underlay` (#161).

Closes #163.

## Files Changed

- `src/sketch.cpp`, `src/sketch.h` — slim coordinator (~400 lines)
- `src/sketch_edge.*`, `src/sketch_edges.*` — edge type and persistent list
- `src/sketch_topo.*` — face extraction and interior splits
- `src/sketch_dims.*` — length dimensions and typed input
- `src/sketch_tools.*` — interactive draw session (tmp edges, finalize/cancel)
- `src/sketch_node_marks.*`, `src/sketch_ais.*` — node marks and AIS helpers
- `src/sketch_display.cpp` — visibility, edge style, sketch switching, list hover
- `src/sketch_operations.cpp` — operation axis, mirror, revolve
- `src/sketch_json.cpp`, `src/sketch_delta.cpp`, `src/utl_types.h`
- `tests/sketch_tests.cpp`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/163
- PR: https://github.com/trailcode/EzyCad/pull/164
- Builds on: #161 / PR #162 (`Sketch_underlay`)

## Test Plan

- [x] Build `EzyCad_lib` and `EzyCad_tests` (Release).
- [x] Run `EzyCad_tests --gtest_filter=Sketch_test.*` — 53/53 passed.
- [ ] Manual smoke: add line, square, circle, arc; undo/redo; mirror/revolve with operation axis.
- [x] No user-doc updates required (internal refactor).

## Notes

- `Sketch_tools::finalize_edges_` intentionally stays in tools (session commit), not `Sketch_topo` (graph primitives).
- Display/operations split uses dedicated `.cpp` files only (lighter than `Sketch_display` / `Sketch_operations` classes).
- Follow-up: deduplicate batch edge commit with `Sketch_edges::add_edge_impl_`.

## Post-merge

- [ ] Move drafts to `archive/`.
- [ ] Consider `CHANGELOG.md` `[Unreleased]` entry for internal refactor.
