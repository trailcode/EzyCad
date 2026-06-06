# Read the Docs maintenance

All documentation source lives in the single top-level **`docs/`** directory (the only place you need to look for docs-related files).

Author conventions: see **[ezycad_doc_style.md](ezycad_doc_style.md)**.

Published site: **https://ezycad.readthedocs.io/**

## Repository automation

| Mechanism | What it does |
| --- | --- |
| [`.readthedocs.yaml`](../.readthedocs.yaml) | RTD build config (points at `docs/conf.py`). |
| [`.github/workflows/docs.yml`](../.github/workflows/docs.yml) | GitHub Actions verification build (runs on changes under `docs/`). |

Screenshots and diagrams live in `docs/images/`. Icons come from `res/icons/`.

Use Markdown image syntax only: `![alt](images/foo.png)` or `![alt](res/icons/Bar.png)`. Raw HTML `<img>` will not be rewritten by Sphinx.

## Read the Docs dashboard

Configure at [EzyCad admin](https://app.readthedocs.org/dashboard/ezycad/):

1. **Integrations** — Connect GitHub; fix the webhook so every push to `main` triggers a build.
2. **Settings → Builds** — Enable **Build pull requests** for preview URLs on PRs.
3. **Settings → Versions** — Optional: activate **stable** from release tags; **latest** tracks the default branch.

## Local build

```bash
pip install -r docs/requirements.txt
sphinx-build -b html -W docs docs/_build
```

Open `docs/_build/index.html`.
