# Agent Instructions

Pointer for AI coding assistants. Details live in [agents/README.md](agents/README.md).

**Precedence:** user instructions > nested AGENTS.md > this file.

## Always (editing code)

- [agents/conventions/ascii-source.md](agents/conventions/ascii-source.md) for `src/` and `tests/`
- [docs/ezycad_code_style.md](docs/ezycad_code_style.md) for C++ style

## When needed

- UI/settings/docs changes: [agents/conventions/user-docs-sync.md](agents/conventions/user-docs-sync.md)
- Build/test: [agents/workflows/local-dev.md](agents/workflows/local-dev.md) or root README
- Release (maintainers only): [agents/workflows/release.md](agents/workflows/release.md)
- Open work context: one file under [agents/drafts/](agents/drafts/) if relevant

**Token budget:** [agents/conventions/token-lean.md](agents/conventions/token-lean.md)

IDE-specific rules stay out of the repo (see `agents/conventions/token-lean.md`).
