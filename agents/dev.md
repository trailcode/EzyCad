# Development / Local Testing Notes

Short notes for local development and testing tasks, especially around documentation.

## Documentation

1. Test locally:

```bash
pip install -r docs/requirements.txt
sphinx-build -b html -W docs docs/_build
```

Then open `docs/_build/index.html`.
