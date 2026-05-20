# EzyCad documentation style

Use this guide when editing **user-facing Markdown** at the repository root (`usage.md`, `usage-*.md`, `scripting.md`, `README.md`, `CHANGELOG.md`). For C++ in `src/`, see **[ezycad_code_style.md](ezycad_code_style.md)**.

Published HTML: **[https://ezycad.readthedocs.io/](https://ezycad.readthedocs.io/)** (`/en/latest/` tracks `main`). Build plumbing is described in **[docs/readthedocs.md](docs/readthedocs.md)** (Read the Docs dashboard, local `sphinx-build`).

## Canonical files (edit these)

| File | Purpose |
|------|---------|
| `usage.md` | Main usage guide |
| `usage-sketch.md` | 2D sketching |
| `usage-settings.md` | Settings pane and JSON |
| `usage-occt-view.md` | 3D viewer (OCCT) |
| `scripting.md` | Lua / Python consoles |
| `README.md` | Project overview |
| `CHANGELOG.md` | Release notes ([Keep a Changelog](https://keepachangelog.com/)) |

Do **not** edit generated copies under `docs/` for content. At build time, `docs/conf.py` syncs root guides plus `res/icons/` and `doc/gen/` into `docs/` (see `docs/.gitignore`).

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
| Screenshots | `doc/gen/name.png` | `![alt](doc/gen/name.png)` |

Rules:

- Use **Markdown image syntax only**. Do **not** use raw HTML `<img src="...">` — Sphinx leaves those paths unchanged and icons **break** on Read the Docs.
- Commit new PNGs under `res/icons/` or `doc/gen/` with the guide change.
- After image or guide edits, run `sphinx-build -b html -W docs docs/_build` locally, or confirm the Read the Docs build is green (`fail_on_warning: true` in [`.readthedocs.yaml`](.readthedocs.yaml)).

## Links from the application

- **Help → Usage Guide** should open Read the Docs, e.g. `https://ezycad.readthedocs.io/en/latest/usage.html`, not a GitHub blob URL.
- In-app help tooltips (Settings **?** buttons, etc.) should use the same base URL and heading anchors (e.g. `#view-roll`).
- When you rename a heading, check for broken anchors in `src/` (grep for `readthedocs.io` and old `#fragments`).

## Build and CI (reference)

| Item | Location |
|------|----------|
| Read the Docs config | [`.readthedocs.yaml`](.readthedocs.yaml) |
| Sphinx | [`docs/conf.py`](docs/conf.py), [`docs/index.rst`](docs/index.rst) |
| Python deps | [`docs/requirements.txt`](docs/requirements.txt) |
| GitHub Actions | [`.github/workflows/docs.yml`](.github/workflows/docs.yml) |

```bash
pip install -r docs/requirements.txt
sphinx-build -b html -W docs docs/_build
```

## Related docs

- **[ezycad_code_style.md](ezycad_code_style.md)** — C++ style, versioning (`version.h`, `CHANGELOG.md`).
- **[docs/readthedocs.md](docs/readthedocs.md)** — RTD integrations, webhooks, PR previews.
