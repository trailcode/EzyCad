# Agent / AI assistant notes

Short, **repo-local** hints for human developers and AI coding assistants.

These notes can be referenced directly by AI tools or copied into an assistant's project rules / custom instructions as needed.

Root markers [AGENTS.md](../AGENTS.md) and [agents.md](../agents.md) (at the repository root) exist for better auto-discovery by different AI coding tools. They both point here.

## Read this first

1. [workflows/local-dev.md](workflows/local-dev.md) — build, test, format, wasm
2. [conventions/ascii-source.md](conventions/ascii-source.md) + [docs/ezycad_code_style.md](../docs/ezycad_code_style.md) — C++ style
3. [conventions/user-docs-sync.md](conventions/user-docs-sync.md) + [docs/ezycad_doc_style.md](../docs/ezycad_doc_style.md) — when UI/settings change
4. [drafts/](drafts/) — issue/PR working notes for a specific task (if any)
5. [workflows/release.md](workflows/release.md) — maintainers cutting a version

## Index

| Path | Intent |
| --- | --- |
| *(repo root)* [AGENTS.md](../AGENTS.md) / [agents.md](../agents.md) | Root-level markers that point AI tools at this `agents/` directory. |
| **Conventions** | |
| [conventions/ascii-source.md](conventions/ascii-source.md) | ASCII-only comments and strings in `src/` and `tests/`; points at `ezycad_code_style.md` and `scripts/check-nonascii-src.ps1`. |
| [conventions/user-docs-sync.md](conventions/user-docs-sync.md) | When user-visible features change: which `docs/` files and JSON tables to update. |
| [conventions/github-drafts.md](conventions/github-drafts.md) | GitHub-first draft naming (`gh-NNN-...`), frontmatter, active/archive lifecycle. |
| **Workflows** | |
| [workflows/local-dev.md](workflows/local-dev.md) | Desktop build, wasm, tests, formatting, ASCII check. |
| [workflows/docs-build.md](workflows/docs-build.md) | Local Sphinx / Read the Docs build. |
| [workflows/release.md](workflows/release.md) | Version bump, CHANGELOG, tag, CI release, wasm demo publish. |
| **Drafts** | |
| [drafts/issues/](drafts/issues/) | GitHub issue drafts (`active/` and `archive/`). |
| [drafts/prs/](drafts/prs/) | GitHub PR drafts (`active/` and `archive/`). |
| **Outreach** | |
| [outreach/discoverability.md](outreach/discoverability.md) | Draft posts for forums, Reddit, awesome lists. |
| **Full style guides (in docs/)** | |
| [docs/ezycad_code_style.md](../docs/ezycad_code_style.md) | Coding style, conventions, and best practices. |
| [docs/ezycad_doc_style.md](../docs/ezycad_doc_style.md) | User guides, Read the Docs, images, in-app doc URLs. |
| [docs/building-occt.md](../docs/building-occt.md) | Download/build OCCT for Windows desktop and wasm. |

When using Grok Build (or other tools with personal/global instruction sets such as `C:\agents\` or `~/.grok/`), also consult those alongside the notes here.
