# Agent Instructions

This file serves as a root-level marker for AI coding assistants. It points to the dedicated `agents/` directory for short, repo-local notes, conventions, and guidance.

## Overview
- See [agents/README.md](agents/README.md) for the index of notes.
- This project keeps IDE-specific configuration and rule files out of the repository.
- Deeper-nested instruction files (AGENTS.md, agents.md, etc. in subdirectories) take precedence over this one.
- Direct instructions from the user always take highest precedence.

## Key Resources in agents/
- `agents/dev.md`: Local development and testing commands (e.g. Sphinx documentation builds).
- `agents/ezycad-ascii-source.md`: ASCII-only rules for files under `src/`.
- `agents/discoverability-outreach.md`: Notes for posts, discoverability, and outreach.
- Style guides (in `docs/`): `ezycad_code_style.md` (C++ code) and `ezycad_doc_style.md` (user-facing documentation).
- `agents/issues/` and `agents/prs/`: Per-issue and per-PR working notes and templates.
- Other files listed in `agents/README.md`.

## How to Use
- When working on EzyCad code, start by reading `agents/README.md` and `agents/dev.md`.
- Follow the coding style in `docs/ezycad_code_style.md` and the documentation style in `docs/ezycad_doc_style.md`.
- Check `agents/issues/` or `agents/prs/` for context on specific work items.

## Project Rules
- Coding conventions, build/test instructions, and other guidance live in the files under `agents/` and in `docs/`.
- When editing files deeper in the tree, check for more specific AGENTS.md or agents.md files in those directories or their parents.

Update these notes as the project evolves.