---
github_issue: 192
github_pr: 193
status: active
paired_draft: ../prs/active/gh-193-align-markdown-tables.md
---

# Align Markdown tables for source + preview readability

**Suggested labels:** `documentation`, `enhancement`

---

## Title (GitHub)

Align Markdown tables for source + preview readability

## Body (GitHub)

### Summary

Prefer aligned GFM pipe tables across repo Markdown so tables stay readable in editor/source view and still render correctly in preview / GitHub / Read the Docs. Add an agent convention and a small re-align helper script.

### Problem

- Many `.md` tables used compact `| --- |` separators without column padding, so source/"code" view was hard to scan.
- There was no documented preference for agents/contributors on table formatting, or guidance for wide tables vs editor word wrap.

### Implemented scope

**Tooling:**

- `scripts/align_md_tables.py` — aligns GFM pipe tables (skips fenced code blocks and build/vendor trees); supports `--check`.

**Documentation / agent notes:**

- `agents/conventions/markdown-tables.md` — preferred form, wide-table/word-wrap notes, helper usage
- Pointers from `AGENTS.md` / `agents.md`, `agents/README.md`, `token-lean.md`, `local-dev.md`, `docs/ezycad_doc_style.md`
- Existing tables realigned in `docs/`, `src/doc/`, `agents/`, and root `README.md`

### Acceptance criteria

- [x] GFM tables in tracked `.md` files are column-aligned (helper `--check` clean)
- [x] Agent convention documents preferred form and word-wrap guidance
- [x] Helper script documented in local-dev / agent index
- [ ] CI green on the linked PR

### Files touched

- `scripts/align_md_tables.py`
- `agents/conventions/markdown-tables.md`
- `agents.md` / `AGENTS.md`, `agents/README.md`, `agents/conventions/token-lean.md`, `agents/workflows/local-dev.md`
- `docs/ezycad_doc_style.md` and realigned tables under `docs/`, `src/doc/`, `agents/`, `README.md`
- `agents/drafts/issues/active/gh-192-align-markdown-tables.md` (this draft)

### Related

- PR: https://github.com/trailcode/EzyCad/pull/193

### Test plan

- [x] `python scripts/align_md_tables.py --check`
- [ ] Spot-check a few pages in Markdown preview (e.g. `src/doc/shape.md`, `docs/usage-sketch.md`)
- [ ] Optional: `sphinx-build -b html -W docs docs/_build` if docs CI does not already cover it
