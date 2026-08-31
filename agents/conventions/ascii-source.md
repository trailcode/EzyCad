# ASCII-only source text (EzyCad)

Use this as a **Cursor rule** or paste into your assistant context when editing `src/` or `tests/` (EzyCad_tests sources).

In `src/` and `tests/`, keep **comments and string literals 7-bit ASCII** (no Unicode punctuation or symbols: smart quotes, en/em dashes, arrows, ellipsis, etc.). Use ASCII equivalents (`-`, `...`, `->`, `sqrt(2)`, plain `'`).

Project style: [docs/ezycad_code_style.md](../../docs/ezycad_code_style.md) (sections **Formatting** / line endings and **Source encoding**). After editing or creating `src/` or `tests/` C++, run `python scripts/agent_check.py <touched paths>` (ASCII + code style in one process). Humans/CI can still use `scripts/check-nonascii-src.ps1` or `check-nonascii-src.cmd`.
