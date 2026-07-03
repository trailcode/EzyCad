# Agent Instructions

This file serves as a root-level marker for AI coding assistants. It points to the dedicated `agents/` directory for short, repo-local notes, conventions, and guidance.

## Overview

- See [agents/README.md](agents/README.md) for the full index and read order.
- This project keeps IDE-specific configuration and rule files out of the repository.
- Deeper-nested instruction files (AGENTS.md, agents.md, etc. in subdirectories) take precedence over this one.
- Direct instructions from the user always take highest precedence.

## Read order

1. [agents/workflows/local-dev.md](agents/workflows/local-dev.md)
2. [agents/conventions/ascii-source.md](agents/conventions/ascii-source.md) and [docs/ezycad_code_style.md](docs/ezycad_code_style.md)
3. [agents/conventions/user-docs-sync.md](agents/conventions/user-docs-sync.md) when changing user-visible behavior
4. [agents/drafts/](agents/drafts/) for context on a specific issue or PR
5. [agents/workflows/release.md](agents/workflows/release.md) for version releases (maintainers)

Update these notes as the project evolves.
