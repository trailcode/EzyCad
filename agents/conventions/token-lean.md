# Keeping agent context lean

Goal: give assistants **only what they need** for the task at hand. Full style guides stay in `docs/`; load them when editing those files.

## Default load (most tasks)

1. Root [AGENTS.md](../../AGENTS.md) — pointers only (~20 lines).
2. [ascii-source.md](ascii-source.md) — when touching `src/` or `tests/`.
3. [docs/ezycad_code_style.md](../../docs/ezycad_code_style.md) — when writing C++ (do not duplicate in chat).

**Do not** auto-load: `workflows/release.md`, `outreach/`, `drafts/archive/`, or full `local-dev.md` unless building/releasing.

## Load on demand

| Task | Read |
| --- | --- |
| Build / test | [workflows/local-dev.md](../workflows/local-dev.md) — or root [README.md](../../README.md#building-instructions) |
| User-visible UI/settings | [user-docs-sync.md](user-docs-sync.md) + target `docs/usage-*.md` only |
| Sketch subsystem (`src/sketch*`, `tests/sketch_tests.cpp`, sketch behavior in `occt_view` / `gui`) | [src/doc/sketch.md](../../src/doc/sketch.md) — read before editing; update when API, invariants, module layout, or workflows change |
| Shape module (`src/shp*`, shape ops in `occt_view` / `gui`) | [src/doc/shape.md](../../src/doc/shape.md) — read before editing; update when API, operation patterns, or registration/undo change |
| GUI / viewer shell (`src/gui*`, input routing, settings panes) | [src/doc/gui.md](../../src/doc/gui.md) — read before editing; update when modes, Options, hotkeys, or settings keys change |
| Docs build | [workflows/docs-build.md](../workflows/docs-build.md) |
| Release | [workflows/release.md](../workflows/release.md) |
| Specific issue/PR | One file under `drafts/issues/active/` or `drafts/prs/active/` |
| Forum posts | [outreach/discoverability.md](../outreach/discoverability.md) |

## Draft hygiene (saves tokens long-term)

- **Archive** merged drafts; keep `active/` small (open work only).
- Use **frontmatter** + short bodies in archive files (see [github-drafts.md](github-drafts.md)).
- Prefer **GitHub issue/PR links** over pasting long draft bodies into rules.
- Drop scratch files (`*-pr-body.txt`, duplicate templates).

## Developer docs in `src/doc/`

Module notes live under `src/doc/` (IDE folder `src\doc`; see root `CMakeLists.txt`). They are **not** user guides — do not duplicate `docs/usage-*.md`.

| Module doc | When |
| --- | --- |
| [src/doc/sketch.md](../../src/doc/sketch.md) | Editing `src/sketch*`, `tests/sketch_tests.cpp`, or sketch-facing behavior in `occt_view` / `gui` / `mode` |
| [src/doc/shape.md](../../src/doc/shape.md) | Editing `src/shp*`, shape operations in `occt_view` / `gui`, or 3D solid behavior |
| [src/doc/gui.md](../../src/doc/gui.md) | Editing `src/gui*`, `Occt_view` in `gui_occt_view*`, input routing, Options/Settings UI |

**On sketch changes:** read `sketch.md` first for context. Update it in the same branch when you change public `Sketch` API, coordinator/sub-module boundaries, invariants (IDs, undo, transient vs committed state), or developer usage patterns. Skip updates for internal-only refactors with no doc impact.

**On shape changes:** read `shape.md` first for context. Update it when you change `Shp` API, `Shp_operation_base` contract, operation class behavior, undo/finalize patterns, or how shapes are registered in `Occt_view`. Skip updates for internal-only refactors with no doc impact.

**On GUI changes:** read `gui.md` first for context. Update it when you change input routing, mode/options panels, toolbar commands, settings JSON keys, or pane visibility rules. Sync user docs per `user-docs-sync.md` when behavior is user-visible.

## Cursor / IDE rules

Keep personal rules minimal: point at `AGENTS.md` and `agents/conventions/ascii-source.md`. Avoid copying whole `local-dev.md` or `release.md` into global rules.

## What stays out of the repo

IDE configs (`.cursor/rules/`, etc.) — use repo `agents/` as the portable source of truth.
