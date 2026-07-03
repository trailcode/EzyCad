# Documentation build (Read the Docs / Sphinx)

```bash
pip install -r docs/requirements.txt
sphinx-build -b html -W docs docs/_build
```

Then open `docs/_build/index.html`.

See also [docs/readthedocs.md](../../docs/readthedocs.md) for RTD maintenance and conventions.

When user-visible behavior changes, follow [agents/conventions/user-docs-sync.md](../conventions/user-docs-sync.md) in the same branch/PR as the code.
