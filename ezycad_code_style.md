# EzyCad coding style

Use this style when editing or adding C/C++ code in the EzyCad project (files under `src/`, not third_party or build output).

## Headers

- Use **`#pragma once`** for include guards (no `#ifndef`/`#define`/`#endif`).
- **Include order**: Put the **closest / most direct** dependencies first.
  - In a **`.cpp`** file: include **this file's own header first** (e.g. in `sketch_nodes.cpp`, the first line is `#include "sketch_nodes.h"`). Then other project headers it needs, then external/system headers (`<...>`). Use a blank line before the `<...>` block when you have both.
  - In a **`.h`** file: include the project headers this file directly depends on first (e.g. `sketch_nodes.h` has `#include "types.h"` before `<gp_Pln.hxx>`; `shp_operation.h` has `"shp.h"`, `"types.h"` first). Then external/system headers. So **project (`"..."`) before external (`<...>`)**, with the most relevant project header at the very top when there is one.
- Prefer **forward declarations** for types used only as pointers/references (e.g. `class gp_Pln;`) when it reduces includes.

## Naming

- **Classes / structs**: PascalCase with underscores, e.g. `Sketch_nodes`, `Shp`, `Sketch_AIS_edge`, `Occt_view`.
- **Enums**: PascalCase; enum values either PascalCase or descriptive with underscores (e.g. `Full`, `Background`, `Sketch_add_node`, `_count` for sentinel).
- **Member variables**: `m_` prefix (e.g. `m_nodes`, `m_view`). Static members: `s_` prefix (e.g. `s_snap_dist_pixels`).
- **Constants** (e.g. lookup arrays for enums): `c_` prefix (e.g. `c_mode_strs`, `c_chamfer_mode_strs`).
- **Functions / methods**: snake_case (e.g. `add_new_node`, `get_node_exact`, `try_get_node_idx_snap`).
- **Private methods**: snake_case with trailing underscore (e.g. `update_node_snap_anno_`, `try_snap_outside_`).
- **Type aliases**: snake_case with suffix by role, e.g. `*_ptr` for handles (`AIS_Shape_ptr`, `Shp_ptr`), `*_rslt` for result types (`Shp_rslt`). Typedefs like `ScreenCoords` are PascalCase.
- **Macros**: UPPER_SNAKE_CASE (e.g. `EZY_ASSERT`, `EZY_ASSERT_MSG`, `DBG_MSG`).

## Formatting

- **Indentation**: 2 spaces (no tabs).
- **Access specifiers**: ` public:` and ` private:` (one space before `public`/`private`/`protected`).
- **Braces**: Opening brace for class/struct on the same line. For **control flow** (`if`, `for`, `while`, `switch`), put the opening brace on the **next line**. For functions, opening brace often on the next line; **short functions** (e.g. single return) may be on one line—`.clang-format` (AllowShortFunctionsOnASingleLine: All) does this automatically. `.clang-format` uses `BraceWrapping.AfterControlStatement: Always` to enforce control-statement brace placement.
- **Alignment**: Align member declarations in columns when it aids readability (type and name aligned across lines in the same block).
- **Initialization**: Prefer brace-initialization for members (e.g. `bool is_midpoint {false};`, `size_t m_prev_num_nodes {0};`).
- **Local declarations**: Prefer declaring locals close to first use for readability. For values shared by a render block, compute them once immediately before that block.
- **Short control flow**: Single-line `if`/`for` without braces is acceptable when the body is a single statement; use braces for multi-line or nested bodies.
- Use **`// clang-format off`** / **`// clang-format on`** only where layout must be preserved (e.g. macro-like blocks, tables). Prefer running clang-format; it is the source of truth for formatting.

## Versioning and releases

- **Product version** lives in **`src/version.h`**: `EZYCAD_VERSION_MAJOR`, `EZYCAD_VERSION_MINOR`, `EZYCAD_VERSION_PATCH`, and `EZYCAD_VERSION_STRING`. Follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html) (MAJOR.MINOR.PATCH). The header is hand-maintained so it stays valid regardless of build system (CMake or otherwise).
- **Release notes** live in **`CHANGELOG.md`** at the repo root, using [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) sections (`Added`, `Changed`, `Fixed`, etc.). Add notable work under **`[Unreleased]`** as you go; when you cut a release, rename that section to **`[x.y.z] - YYYY-MM-DD`**, bump **`version.h`** to match, and create a git tag **`vx.y.z`** consistent with `EZYCAD_VERSION_STRING`.
- **Do not confuse** the product version with **`k_settings_version`** in `gui_settings.cpp` (and `version` in saved settings JSON): that value is the **settings file format** version, not the application version.

## Code organization

- **Reader-first order** (`.cpp`): Arrange functions from high-level behavior to lower-level detail so the first screen shows the primary workflow before helper details.
- **Helper placement**: Prefer keeping narrow file-local helpers close to the functions that use them. Avoid large top-of-file helper blocks unless helpers are broadly reused across the file.
- **Templates**: Prefer putting template implementations in `.inl` files included from the header (e.g. `types.inl`, `utl.inl`).
- **OCCT handles**: Use `opencascade::handle<T>` and project aliases (e.g. `AIS_Shape_ptr`, `Shp_ptr`).
- **Result/error handling**: See **Fail fast** below. Use `Result<T>` and `Status` from `utl.h`, `CHK_RET(...)` for early return on failure.
- **Assertions**: See **Fail fast** below. Use `EZY_ASSERT` and `EZY_ASSERT_MSG` from `dbg.h`; use `DBG_MSG` for debug logging.

## Don't Repeat Yourself (DRY)

- DRY is important, but it is guidance, not a strict rule.
- **Reader-first pairing**: Pragmatic DRY matches **Reader-first order** in **Code organization** above. Prefer a self-contained workflow or UI pane you can read top-to-bottom over distant abstractions whose only win is deleting a few repeated lines.
- Avoid hasty abstractions: do not generalize too early before patterns are stable.
- Too much DRY can increase coupling by forcing unrelated code through one shared abstraction.
- Prefer readability over clever reuse when repetition is small and explicit code is clearer.
- **When duplication is acceptable**: a few lines repeated across nearby UI or glue code can beat a shared helper if call sites are likely to diverge (different tooltips, widths, or disabled logic) or if extracting would scatter one screen across many symbols. Prefer **locality**: keep a small block self-contained so a reader does not jump to understand one pane.
- **Rule of thumb**: extract when the behavior is **the same and stable** (or when a bug fix must touch N copies); wait when the pattern is still moving. If you extract, prefer a **narrow file-local helper** next to its users over a generic framework.
- Context matters: stronger DRY is often good in monolith/shared-library code; some duplication can be healthier in fast-changing or separated systems.
- Balance DRY with KISS, YAGNI, and overall cognitive load.

## Fail fast

EzyCad uses **fail fast**: detect problems as early as possible. Do not hide programmer mistakes with silent fallbacks (e.g. substituting empty defaults when a required dependency is missing) unless that behavior is an explicit, documented product decision.

Separate **who is responsible** for the fault:

### Programmer errors (use asserts)

Use **`EZY_ASSERT`** / **`EZY_ASSERT_MSG`** (`dbg.h`) for conditions that must hold if the code and call patterns are correct:

- Preconditions that callers are required to satisfy (e.g. an object that is always constructed before use in normal app flow).
- Internal invariants, impossible `else` branches after exhaustive handling, index bounds that prove from prior logic.
- “This should never happen” after a switch or state machine where all cases were meant to be covered.

These are **not** expected at runtime in correct usage; they signal a bug to fix. Do not use asserts for user input, files, or the operating system.

### User mistakes and external failures (return errors or status)

Use **`Status`**, **`Result<T>`**, **`CHK_RET`**, or other explicit error paths for faults that can happen in **valid** programs:

- **User errors**: wrong mode, nothing selected when something must be selected, invalid input — typically `Status::user_error("...")` or an appropriate `Result_status` with a clear message.
- **OS and I/O**: file missing, **disk full**, permission denied, read/write failures — return or propagate failure; log or surface a message as the UI already does elsewhere. **Never** assert on these.
- **Modeling / OCCT / topology**: operation failed in the kernel — use `Status` / `Result` and existing result categories (e.g. `Topo_error`) as fits the call site.

Prefer **`CHK_RET(expr)`** when a callee returns `Status` or `Result<T>` and the caller should abort on failure.

### Quick reference

| Kind of problem | Mechanism |
|-----------------|-----------|
| Bug, contract violation, impossible branch | `EZY_ASSERT` / `EZY_ASSERT_MSG` |
| User error, I/O, OS, recoverable external failure | `Status`, `Result<T>`, `CHK_RET`, explicit error return |

## Comments

- Use `//` for line comments.
- Document non-obvious parameters or lifetime requirements (e.g. `// \`view\` must exist for the lifetime of this object`).
- Prefer brief inline comments for non-obvious logic; keep comments in sync with code.

## Source encoding (ASCII only)

- **Comments, string literals, and documentation** in `src/` must be **7-bit ASCII** only (code points U+0000 through U+007F). Do not use Unicode punctuation or symbols (e.g. en dash, em dash, ellipsis, arrows, multiplication sign, middle dot, math letters like theta) or curly/smart apostrophes.
- Use ASCII substitutes instead, for example:
  - Ranges: `0-255`, `1-9` (hyphen), not en dash.
  - Punctuation in prose: `-` for dash; `...` for ellipsis; `->` for “maps to” / arrows in comments; plain `'` for apostrophes.
  - Math in comments: spell out (`sqrt(2)`, `theta`) or use ASCII operators (`x` for cross-product context, `*` for multiply).
- Run `scripts/check-nonascii-src.ps1` (or `check-nonascii-src.cmd`) before committing when touching `src/`.
- For AI tools: the same rule is summarized in `agents/ezycad-ascii-source.md` (optional; nothing under `.cursor/` is committed).

## C++ usage

- Prefer **`enum class`** for enumerations.
- Use **`std::optional`** where a value may be absent.
- Use **`const`** and **`const&`** for parameters and accessors where appropriate.
- Keep consistent with existing file style (e.g. include order, brace placement) when editing.
