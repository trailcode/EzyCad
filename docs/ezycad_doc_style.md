# EzyCad documentation style

Use this guide when editing **user-facing Markdown** in `docs/` (`usage.md`, `usage-*.md`, `scripting.md`, plus `building-occt.md`, `bugs.md`, the two style guides, etc.) and the root `README.md` / `CHANGELOG.md`. For C++ in `src/`, see **[ezycad_code_style.md](ezycad_code_style.md)**.

Published HTML: **[https://ezycad.readthedocs.io/](https://ezycad.readthedocs.io/)** (`/en/latest/` tracks `main`). Build plumbing is described in **[readthedocs.md](readthedocs.md)**.

## Canonical files (edit these)

All live together under the single `docs/` folder:

| File | Purpose |
|------|---------|
| `usage.md` | Main usage guide |
| `usage-sketch.md` | 2D sketching |
| `usage-settings.md` | Settings pane and JSON |
| `usage-occt-view.md` | 3D viewer (OCCT) |
| `scripting.md` | Lua / Python consoles |
| `building-occt.md` | Building Open CASCADE (desktop + WebAssembly) |
| `bugs.md` | Known issues / sharp edges |
| `ezycad_code_style.md` | C++ style (in `src/`) |
| `ezycad_doc_style.md` | This file – Markdown / user docs style |
| `README.md` (root) | Project overview |
| `CHANGELOG.md` (root) | Release notes ([Keep a Changelog](https://keepachangelog.com/)) |

## Writing conventions

- **Audience**: Machinists and hobby CAD users; prefer plain language and short steps.
- **Headings**: Use `##` / `###` for sections Sphinx/MyST can index; keep a stable **Table of Contents** in `usage.md` when adding major sections.
- **Keyboard shortcuts**: Use `<kbd>Tab</kbd>`, `<kbd>Ctrl</kbd>+<kbd>Z</kbd>`, etc. They render on Read the Docs via MyST.
- **Tables**: GFM pipe tables are fine on Read the Docs.
- **Cross-links**: Link other guides as `usage-sketch.md`, `usage-settings.md#view-menu`, or `#anchor` within the same file. Prefer anchors that match heading text (MyST slugifies headings for URLs).
- **Encoding**: The **ASCII-only** rule in `ezycad_code_style.md` applies to `src/`, not to these guides; Unicode in user docs is acceptable when it helps clarity.

## Images

| Kind | Path | Syntax |
|------|------|--------|
| Toolbar icons | `res/icons/Name.png` | `![alt](res/icons/Name.png)` |
| Screenshots / diagrams | `images/name.png` | `![alt](images/name.png)` |

Rules:

- Use **Markdown image syntax only**. Do **not** use raw HTML `<img src="...">` — Sphinx will not rewrite the path and images will break on Read the Docs.
- Commit new screenshots under `docs/images/`.
- Commit toolbar icons under `res/icons/`.
- After changes, run `sphinx-build -b html -W docs docs/_build` locally, or rely on the Read the Docs build (it has `fail_on_warning: true`).

## Links from the application

- **Help → Usage Guide** should open Read the Docs, e.g. `https://ezycad.readthedocs.io/en/latest/usage.html`, not a GitHub blob URL.
- In-app help (Settings **?** buttons, and the **?** button that appears right after the tool/mode name header in every Options pane) should use the same base URL and heading anchors (e.g. `#shape-move-tool-g`, `#add-node-tool`). The per-mode links are defined in the `get_doc_url_for_mode` map (see `src/gui_mode.cpp` and the Options pane header + help button implementation).
- When you rename a heading, check for broken anchors in `src/` (grep for `readthedocs.io` and old `#fragments`).

## Build and CI (reference)

Everything documentation-related now lives under the single `docs/` folder at the repository root. This is the only directory contributors need to care about for docs.

| Item | Location |
|------|----------|
| Read the Docs config | [`.readthedocs.yaml`](.readthedocs.yaml) (at repo root) |
| Sphinx config + content | `docs/conf.py`, `docs/index.rst`, all the `*.md` guides |
| Python deps | `docs/requirements.txt` |
| GitHub Actions verification | [`.github/workflows/docs.yml`](.github/workflows/docs.yml) |

```bash
pip install -r docs/requirements.txt
sphinx-build -b html -W docs docs/_build
```

## Related docs

- **[ezycad_code_style.md](ezycad_code_style.md)** — C++ style, versioning (`version.h`, `CHANGELOG.md`).
- **[readthedocs.md](readthedocs.md)** — RTD integrations, webhooks, PR previews, local build instructions.
- **[../agents/conventions/user-docs-sync.md](../agents/conventions/user-docs-sync.md)** — Checklist for which user guides and JSON tables to update when features change (for agents and contributors).
