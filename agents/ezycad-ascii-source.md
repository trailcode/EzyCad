# ASCII-only source text (EzyCad)

Use this as a **Cursor rule** or paste into your assistant context when editing `src/` or `tests/` (EzyCad_tests sources).

In `src/` and `tests/`, keep **comments and string literals 7-bit ASCII** (no Unicode punctuation or symbols: smart quotes, en/em dashes, arrows, ellipsis, etc.). Use ASCII equivalents (`-`, `...`, `->`, `sqrt(2)`, plain `'`).

Project style: `ezycad_code_style.md` (sections **Formatting** / line endings and **Source encoding**). Verify with `scripts/check-nonascii-src.ps1` or `scripts/check-nonascii-src.cmd`.
