# Agent / AI assistant notes

Repo-local hints for developers and AI assistants. Keep context small: see [conventions/token-lean.md](conventions/token-lean.md).

Root markers: [AGENTS.md](../AGENTS.md) / [agents.md](../agents.md).

## Quick index

| Need                         | File                                                                                                                 |
| ---------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| ASCII / `src/` edits         | [conventions/ascii-source.md](conventions/ascii-source.md)                                                           |
| C++ style (full)             | [docs/ezycad_code_style.md](../docs/ezycad_code_style.md)                                                            |
| User docs when UI changes    | [conventions/user-docs-sync.md](conventions/user-docs-sync.md)                                                       |
| Sketch module (dev doc)      | [src/doc/sketch.md](../src/doc/sketch.md) — read when editing sketch code; update if API/architecture changes        |
| Shape module (dev doc)       | [src/doc/shape.md](../src/doc/shape.md) — read when editing `shp_*` code; update if API/operations change            |
| GUI module (dev doc)         | [src/doc/gui.md](../src/doc/gui.md) — read when editing `gui_*` / viewer shell; update if routing or settings change |
| Script module (dev doc)      | [src/doc/script.md](../src/doc/script.md) — read when editing `scr_*`; update if bindings change                     |
| Utility module (dev doc)     | [src/doc/utility.md](../src/doc/utility.md) — read when editing `utl_*`; update if shared helpers or I/O change      |
| Build / test / wasm          | [workflows/local-dev.md](workflows/local-dev.md)                                                                     |
| OCCT desktop 8 vs wasm 7.9.3 | [conventions/occt-wasm-dual-version.md](conventions/occt-wasm-dual-version.md) — until wasm works on OCCT 8          |
| OCCT handles (`*_ptr`)       | [conventions/occt-handles.md](conventions/occt-handles.md) — prefer aliases over `Handle()` for clang-format         |
| Release                      | [workflows/release.md](workflows/release.md)                                                                         |
| Issue/PR drafts              | [drafts/](drafts/) — [github-drafts.md](conventions/github-drafts.md)                                                |
| Token-saving rules           | [conventions/token-lean.md](conventions/token-lean.md)                                                               |
| Markdown tables              | [conventions/markdown-tables.md](conventions/markdown-tables.md) — align GFM pipes for source + preview              |
| Outreach (optional)          | [outreach/discoverability.md](outreach/discoverability.md)                                                           |

Full user-doc style: [docs/ezycad_doc_style.md](../docs/ezycad_doc_style.md). OCCT build: [docs/building-occt.md](../docs/building-occt.md).
