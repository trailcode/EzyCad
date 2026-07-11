# Markdown tables (preferred)

Prefer **aligned GFM pipe tables** in all repo `.md` files so tables stay readable in editor/source ("code") view and render correctly in Markdown preview / GitHub / Read the Docs ("pretty") view.

## Preferred form

```markdown
| Helper                           | Requirement                                     |
| -------------------------------- | ----------------------------------------------- |
| `ensure_operation_shps_()`       | One or more selected `Shp` objects              |
| `ensure_operation_multi_shps_()` | Two or more selected shapes (fuse, cut, common) |
```

Rules:

- Use GFM pipes (`| ... |`), not HTML `<table>`.
- Pad cells with spaces so column `|` characters line up in the source.
- Include a separator row (`| --- | --- |`); pad dashes to the column width.
- Keep each cell on one line; put long notes in a list under the table instead of multiline cells.
- Leave a blank line before and after the table; do not indent table rows (indentation can turn them into a code block).
- Skip tables inside fenced code blocks (examples stay as authored).

## Wide tables and editor word wrap

Aligned pipe rows are **one long line per row**. Pretty/preview mode reflows cells; source/"code" mode does not. If the editor **word-wraps**, column `|` alignment looks broken even though the Markdown is fine.

**Preferred fix (editor):** turn word wrap **off** for Markdown so wide tables scroll horizontally and stay aligned. In Cursor/VS Code user `settings.json`:

```json
"[markdown]": {
  "editor.wordWrap": "off"
}
```

Toggle for the current file: View → Word Wrap (or the equivalent command).

**Content mitigations** (when a table is still painful even with wrap off):

- Prefer **2–3 columns**; split a wide reference into two tables or a short table + bullets.
- Keep cells short; move long prose **under** the table.
- Do **not** rely on multiline pipe cells — GFM does not wrap cell text across source lines cleanly.

Padding/alignment can make rows slightly longer; that is intentional for source readability when wrap is off. Do not un-align tables just to fight wrap.

## Re-align helper

```bash
python scripts/align_md_tables.py
python scripts/align_md_tables.py --check
```

Skips `third_party/`, local `build*` trees, `_deps`, and similar vendor/output dirs.

## Related

- User-facing docs style: [docs/ezycad_doc_style.md](../../docs/ezycad_doc_style.md) (Tables)
- Issue/PR drafts: [github-drafts.md](github-drafts.md)
