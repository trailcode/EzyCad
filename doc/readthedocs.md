# Read the Docs maintenance

Author conventions (Markdown, images, links): **[ezycad_doc_style.md](../ezycad_doc_style.md)**.

Published site: **https://ezycad.readthedocs.io/**

## Repository automation

| Mechanism | What it does |
| --- | --- |
| [`.readthedocs.yaml`](../.readthedocs.yaml) | RTD build config (Sphinx, Python 3.13). |
| [`.github/workflows/docs.yml`](../.github/workflows/docs.yml) | GitHub Actions: builds docs on push/PR when guides change. |

User guides stay at the **repository root** (`usage.md`, etc.). `doc/conf.py` copies them plus `res/icons/` into `doc/` at build time. Screenshots in guides use paths under `doc/gen/` (committed in this folder).

Use Markdown image syntax `![alt](path)` for icons and screenshots. Raw HTML `<img src="...">` is not rewritten by Sphinx and will break on Read the Docs.

## Read the Docs dashboard

Configure at [EzyCad admin](https://app.readthedocs.org/dashboard/ezycad/):

1. **Integrations** — Connect GitHub; fix the webhook so every push to `main` triggers a build.
2. **Settings → Builds** — Enable **Build pull requests** for preview URLs on PRs.
3. **Settings → Versions** — Optional: activate **stable** from release tags; **latest** tracks the default branch.

## Local build

```bash
pip install -r doc/requirements.txt
sphinx-build -b html -W doc doc/_build
```

Open `doc/_build/index.html`.
