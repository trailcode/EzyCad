# Branch `Trailcode/refactor`: src prefix naming, CMake source groups, explicit sketch recorder

**Suggested labels:** `refactor`, `sketch`, `enhancement`

**Branch:** `Trailcode/refactor` (5 commits ahead of `main` at draft time)

---

## Title (GitHub)

Rename `src/` files with domain prefixes and pass `Sketch_op_recorder` explicitly through sketch edits

## Body (GitHub)

### Summary

Tracking branch **`Trailcode/refactor`**: adopt a **prefix-based naming scheme** for flat `src/` files so related modules group together in the IDE and in reviews; organize Visual Studio / CMake **source groups** by prefix; and **remove the implicit `Sketch::m_undo_recorder` back-pointer** in favor of passing `Sketch_op_recorder&` (or `nullptr`) through sketch finalize, add-edge, split, and arc helpers. Includes minor settings API cleanup and consolidation of sketch OCCT handle typedefs into `utl_types.h`.

No user-visible behavior change is intended; this is structural cleanup on top of the delta-based sketch undo work from issue #144 / PR #145.

### Problem

- Generic filenames (`types.h`, `geom.h`, `settings.h`, `modes.h`, `dbg.h`, `log.h`, `lua_console.h`, etc.) do not sort or filter well in a flat `src/` tree and make cross-module ownership unclear.
- `Sketch_op_recorder` previously registered itself on `Sketch` via `m_undo_recorder`, so finalize helpers consulted a hidden member instead of an explicit parameter — harder to follow and easy to misuse outside an active recorder scope.
- Sketch-specific OCCT handle typedefs lived in `sketch.h` though they are shared typing concerns.
- `settings::load()` duplicated logic now owned by `load_with_defaults()`.

### Implemented scope (on branch)

**File renames (prefix convention):**

| Old                    | New                        | Prefix             |
| ---------------------- | -------------------------- | ------------------ |
| `dbg.h`                | `utl_dbg.h`                | `utl_` utilities   |
| `geom.cpp/h/inl`       | `utl_geom.cpp/h/inl`       | `utl_`             |
| `log.cpp/h`            | `utl_log.cpp/h`            | `utl_`             |
| `ply_io.cpp/h`         | `utl_ply_io.cpp/h`         | `utl_`             |
| `settings.cpp/h`       | `utl_settings.cpp/h`       | `utl_`             |
| `types.h/inl`          | `utl_types.h/inl`          | `utl_`             |
| `modes.cpp/h`          | `gui_modes.cpp/h`          | `gui_`             |
| `lua_console.cpp/h`    | `scr_lua_console.cpp/h`    | `scr_` (scripting) |
| `python_console.cpp/h` | `scr_python_console.cpp/h` | `scr_`             |

Existing prefixes unchanged: `shp_*`, `sketch*`, `gui*`, `occt_*`, `utl_*` (json/occt helpers), `delta*`.

**CMake (`CMakeLists.txt`):**

- Pin `utl_settings.cpp` in the library (replacing `settings.cpp`).
- Split `source_group` for `src/` into `src\shp`, `src\gui`, `src\utl`, `src\sketch`, `src\scr`, and `src` (other).

**Sketch recorder refactor (`src/sketch.cpp`, `src/sketch.h`, `src/sketch_delta.cpp`):**

- Remove `Sketch::m_undo_recorder` and recorder register/unregister on `Sketch`.
- Thread `Sketch_op_recorder&` through `finalize_*`, `add_edge_`, `add_arc_circle_`, and `split_linear_edges_at_node_if_interior_` overloads; optional `Sketch_op_recorder*` for paths that may run without undo recording.
- `finalize_elm()` creates one outer recorder and passes it into mode-specific finalize helpers.

**Types (`src/utl_types.h`, `src/sketch.h`):**

- Move `Sketch_AIS_edge_ptr`, `Sketch_AIS_node_mark_ptr`, and `Sketch_face_shp_ptr` to `utl_types.h`.

**Settings (`src/utl_settings.cpp/h`):**

- Remove public `settings::load()`; fold read + legacy path + default bootstrap into `load_with_defaults()`.

**Includes / tests:**

- Update `#include` paths across `src/` and `tests/` for renamed headers.
- Extend `Sketch_access` test helpers for recorder overloads.

**Documentation:**

- `docs/ezycad_code_style.md` — update examples to new header names (`utl_types.h`, `utl_dbg.h`, `utl_types.inl`).

### Out of scope / follow-ups

- [ ] Add a **File naming** section to `docs/ezycad_code_style.md` documenting `shp_`, `gui_`, `sketch`, `scr_`, `utl_` prefixes (this branch updates examples only).
- [ ] Optional future: physical subdirectories under `src/` matching the CMake source groups.
- [ ] Scan external docs / wiki / fork references for old header names after merge.

### Acceptance criteria

- [ ] All renamed headers compile on native Debug + Release and Emscripten (no stale `#include` of old names).
- [ ] Visual Studio / IDE solution explorer shows prefix-based source groups.
- [ ] Sketch undo/redo still works: linear finalize, arc/circle, crossing splits, mirror cancel path.
- [ ] Settings load/save unchanged for existing user JSON and wasm `localStorage`.
- [ ] `EzyCad_tests` pass (especially `Sketch_test.Undo*` and related cases).
- [ ] No user-facing doc updates required (internal refactor only).

### Files touched

- `CMakeLists.txt`
- `docs/ezycad_code_style.md`
- Renamed: `src/utl_*`, `src/gui_modes.*`, `src/scr_*`
- Updated includes: `src/gui*`, `src/occt_view.*`, `src/shp_*`, `src/sketch*`, `src/utl.h`
- `tests/doc_link_tests.cpp`, `tests/sketch_tests.cpp`
- `agents/drafts/issues/active/draft-src-prefix-refactor-sketch-recorder.md` (this draft)

### Related

- Builds on sketch delta undo: `agents/drafts/issues/archive/gh-144-sketch-delta-undo-redo.md`, `agents/drafts/prs/archive/gh-145-sketch-delta-undo-redo.md`
- Branch: `Trailcode/refactor`
- PR draft: `agents/drafts/prs/active/draft-src-prefix-refactor-sketch-recorder.md`
- GitHub issue / PR links once created.

### Test plan

- [ ] Build `EzyCad_lib` (Debug + Release) and `EzyCad_tests`.
- [ ] Run `EzyCad_tests --gtest_filter=Sketch_test.Undo*:Sketch_test.MirrorSelectedEdges_NoEdgesSelected`.
- [ ] Manual smoke: add linear edge + undo/redo; arc/circle + undo/redo; settings persist after restart.
- [ ] Confirm IDE source groups list `src\utl`, `src\gui`, `src\shp`, etc. (prefix folders in Solution Explorer).
- [ ] `scripts/check-nonascii-src.ps1` if any new comments were added in `src/`.
