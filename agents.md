# Agent Instructions

Pointer for AI coding assistants. Details live in [agents/README.md](agents/README.md).

**Precedence:** user instructions > nested AGENTS.md > this file.

## Always (editing code)

- [agents/conventions/ascii-source.md](agents/conventions/ascii-source.md) for `src/` and `tests/`
- [docs/ezycad_code_style.md](docs/ezycad_code_style.md) for C++ style

## When needed

- UI/settings/docs changes: [agents/conventions/user-docs-sync.md](agents/conventions/user-docs-sync.md)
- Sketch subsystem: [src/doc/sketch.md](src/doc/sketch.md) (read; update when API or architecture changes)
- Shape module: [src/doc/shape.md](src/doc/shape.md) (read; update when API or operation patterns change)
- GUI module: [src/doc/gui.md](src/doc/gui.md) (read; update when input routing, modes, or settings change)
- Script consoles: [src/doc/script.md](src/doc/script.md) (read; update when bindings or console UI change)
- Utilities: [src/doc/utility.md](src/doc/utility.md) (read; update when utl_* contracts or I/O change)
- Script consoles: [src/doc/script.md](src/doc/script.md) (read; update when bindings or console UI change)
- Utilities: [src/doc/utility.md](src/doc/utility.md) (read; update when utl_* contracts or I/O change)
- Build/test: [agents/workflows/local-dev.md](agents/workflows/local-dev.md) or root README
- Release (maintainers only): [agents/workflows/release.md](agents/workflows/release.md)
- Open work context: one file under [agents/drafts/](agents/drafts/) if relevant

**Token budget:** [agents/conventions/token-lean.md](agents/conventions/token-lean.md)

IDE-specific rules stay out of the repo (see `agents/conventions/token-lean.md`).
