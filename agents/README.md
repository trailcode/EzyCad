# Agent / AI assistant notes

Short, **repo-local** hints for human developers and AI coding assistants.

These notes can be referenced directly by AI tools or copied into an assistant's project rules / custom instructions as needed.

Root markers `AGENTS.md` and `agents.md` (at the repository root) exist for better auto-discovery by different AI coding tools. They both point here.

| File | Intent |
| --- | --- |
| *(repo root)* [AGENTS.md](../AGENTS.md) / [agents.md](../agents.md) | Root-level markers that point AI tools at this `agents/` directory. |
| [ezycad-ascii-source.md](ezycad-ascii-source.md) | ASCII-only comments and strings in `src/` and `tests/` (EzyCad_tests sources); points at `ezycad_code_style.md` and `scripts/check-nonascii-src.ps1`. |
| [user-docs-sync.md](user-docs-sync.md) | When user-visible features are added or changed, which `docs/` files and JSON tables to update (Settings, Options, CHANGELOG). |
| [discoverability-outreach.md](discoverability-outreach.md) | Draft posts for forums, Reddit, awesome lists (SEO / backlinks). |
| *(repo root)* [docs/building-occt.md](../docs/building-occt.md) | Download/build OCCT for Windows desktop and wasm. |
| *(now in docs/)* [ezycad_doc_style.md](../docs/ezycad_doc_style.md) | User guides, Read the Docs, images, in-app doc URLs. |
| *(now in docs/)* [ezycad_code_style.md](../docs/ezycad_code_style.md) | Coding style, conventions, and best practices. |
| [dev.md](dev.md) | Local development commands (desktop, wasm, tests, formatting, docs builds). |

When using Grok Build (or other tools with personal/global instruction sets such as `C:\agents\` or `~/.grok/`), also consult those alongside the notes here.
