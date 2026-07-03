# PR - `Trailcode/refactor`

## Title

Prefix-based `src/` file naming, CMake source groups, and explicit `Sketch_op_recorder` threading

## Summary

- **Prefix naming:** Rename generic flat `src/` files to domain prefixes — `utl_*` (types, geom, dbg, log, settings, ply I/O), `gui_modes.*`, `scr_*` (Lua/Python consoles). Update all `#include` paths across `src/` and `tests/`.
- **IDE layout:** CMake `source_group` splits library sources into `src\shp`, `src\gui`, `src\utl`, `src\sketch`, `src\scr`, and `src` (remainder).
- **Sketch recorder:** Remove `Sketch::m_undo_recorder`; pass `Sketch_op_recorder&` (or optional pointer) through finalize, add-edge, split, and arc helpers so undo recording is explicit at each call site.
- **Types / settings:** Move sketch OCCT handle typedefs to `utl_types.h`; drop redundant `settings::load()` in favor of consolidated `load_with_defaults()`.
- **Style doc:** Refresh `docs/ezycad_code_style.md` examples for renamed headers.

Refactors on top of delta-based sketch undo (#144 / #145). No intended user-visible behavior change.

## Files Changed

- `CMakeLists.txt`
- `docs/ezycad_code_style.md`
- Renames: `src/dbg.h` -> `src/utl_dbg.h`, `src/geom.*` -> `src/utl_geom.*`, `src/log.*` -> `src/utl_log.*`, `src/ply_io.*` -> `src/utl_ply_io.*`, `src/settings.*` -> `src/utl_settings.*`, `src/types.*` -> `src/utl_types.*`, `src/modes.*` -> `src/gui_modes.*`, `src/lua_console.*` -> `src/scr_lua_console.*`, `src/python_console.*` -> `src/scr_python_console.*`
- `src/gui.cpp`, `src/gui.h`, `src/gui_add.cpp`, `src/gui_mode.cpp`, `src/gui_settings.cpp`
- `src/occt_view.cpp`, `src/occt_view.h`
- `src/shp_*.cpp`, `src/shp_*.h` (includes only)
- `src/sketch.cpp`, `src/sketch.h`, `src/sketch_delta.cpp`, `src/sketch_json.cpp`, `src/sketch_nodes.cpp`, `src/sketch_nodes.h`, `src/sketch_underlay.cpp`
- `src/utl.h`
- `tests/doc_link_tests.cpp`, `tests/sketch_tests.cpp`
- `agents/drafts/issues/active/draft-src-prefix-refactor-sketch-recorder.md`, `agents/drafts/prs/active/draft-src-prefix-refactor-sketch-recorder.md`

## Related

- Issue: (fill after opening from `agents/drafts/issues/active/draft-src-prefix-refactor-sketch-recorder.md`)
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/refactor
- Prior undo work: `agents/drafts/issues/archive/gh-144-sketch-delta-undo-redo.md`, `agents/drafts/prs/archive/gh-145-sketch-delta-undo-redo.md`

## Test Plan

- [ ] Build `EzyCad_lib` (Debug + Release) and `EzyCad_tests`.
- [ ] Run `EzyCad_tests --gtest_filter=Sketch_test.Undo*:Sketch_test.MirrorSelectedEdges_NoEdgesSelected`.
- [ ] Manual: sketch linear edge add + undo/redo; arc/circle + undo/redo; settings JSON round-trip (native + wasm if applicable).
- [ ] Verify Solution Explorer / IDE source groups show prefix folders (`src\utl`, `src\gui`, etc.).
- [ ] Grep repo for stale includes (`"geom.h"`, `"types.h"`, `"settings.h"`, `"modes.h"`, etc.) — expect none outside history.
- [ ] No user-facing docs or CHANGELOG entry required unless reviewers want a `[Changed]` note for contributors updating forks.

## Notes

- **46 files** changed across 5 commits (`Organize`, `Refactor` x3, `Cleanup`). Mostly renames + include updates; substantive logic change is the sketch recorder threading.
- `finalize_elm()` now owns a single outer `Sketch_op_recorder` and passes it into mode finalize helpers — one undo delta per finalize, same as before.
- `split_linear_edges_at_node_if_interior_(node_idx)` without a recorder delegates to the pointer overload with `nullptr` for non-undo paths.
- Follow-up: document prefix convention in `ezycad_code_style.md` (File naming section); optional physical `src/` subdirs later.

## Post-merge (checklist for the author)

- [ ] Open GitHub issue from `agents/drafts/issues/active/draft-src-prefix-refactor-sketch-recorder.md` and link this PR.
- [ ] Fill issue/PR URLs in both agent drafts.
- [ ] Notify fork maintainers of header renames if any external consumers exist.
- [ ] Consider `[Unreleased]` CHANGELOG entry only if treating contributor-facing renames as notable.
