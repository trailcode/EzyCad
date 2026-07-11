---
github_pr: 193
github_issue: 192
status: active
paired_draft: ../issues/active/gh-192-align-markdown-tables.md
---

# PR - Align Markdown tables and document preferred formatting

## Title

Align Markdown tables and document preferred formatting

## Summary

- Align GFM pipe tables across repo Markdown (`docs/`, `src/doc/`, `agents/`, `README.md`) so columns read cleanly in editor/source view and still render in preview.
- Add preferred convention `agents/conventions/markdown-tables.md` (including wide-table / word-wrap guidance) and wire it into agent indexes and `docs/ezycad_doc_style.md`.
- Add `scripts/align_md_tables.py` (with `--check`) to re-align tables; listed in `agents/workflows/local-dev.md`.

## Files Changed

- `scripts/align_md_tables.py`
- `agents/conventions/markdown-tables.md`
- `agents.md`, `agents/README.md`, `agents/conventions/token-lean.md`, `agents/workflows/local-dev.md`
- `docs/ezycad_doc_style.md`
- Realigned tables in `docs/*`, `src/doc/*`, `agents/*`, `README.md`
- `agents/drafts/issues/active/gh-192-align-markdown-tables.md`
- `agents/drafts/prs/active/gh-193-align-markdown-tables.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/192
- Branch: `Trailcode/doc-table-format`

## Test Plan

- [x] `python scripts/align_md_tables.py --check`
- [ ] Spot-check Markdown preview on a wide table (e.g. `src/doc/shape.md`, `docs/usage-settings.md`)
- [ ] Confirm docs CI / `sphinx-build` if required by existing workflows

## Post-merge

- [ ] Archive issue/PR drafts
- [ ] Close #192 if fully addressed
