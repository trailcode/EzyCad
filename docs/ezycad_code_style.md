# EzyCad coding style

Use this style when editing or adding C/C++ code in the EzyCad project (files under `src/`, not third_party or build output).

## Headers

- Use **`#pragma once`** for include guards (no `#ifndef`/`#define`/`#endif`).
- **Include order**: Put the **closest / most direct** dependencies first.
  - In a **`.cpp`** file: include **this file's own header first** (e.g. in `sketch_nodes.cpp`, the first line is `#include "sketch_nodes.h"`). Then other project headers it needs, then external/system headers (`<...>`). Use a blank line before the `<...>` block when you have both.
  - In a **`.h`** file: include the project headers this file directly depends on first (e.g. `sketch_nodes.h` has `#include "utl_types.h"` before `<gp_Pln.hxx>`; `shp_operation.h` has `"shp.h"`, `"utl_types.h"` first). Then external/system headers. So **project (`"..."`) before external (`<...>`)**, with the most relevant project header at the very top when there is one.
- Prefer **forward declarations** for types used only as pointers/references (e.g. `class gp_Pln;`) when it reduces includes.

## Naming

- **Classes / structs**: PascalCase with underscores, e.g. `Sketch_nodes`, `Shp`, `Sketch_AIS_edge`, `Occt_view`.
- **Enums**: PascalCase; enum values either PascalCase or descriptive with underscores (e.g. `Full`, `Background`, `Sketch_add_node`, `_count` for sentinel).
- **Member variables**: `m_` prefix (e.g. `m_nodes`, `m_view`). Static members: `s_` prefix (e.g. `s_snap_dist_pixels`).
- Prefer clear domain prefixes for related member groups (e.g. `m_underlay_*`) instead of mixed short forms.
- **Constants** (e.g. lookup arrays for enums): `c_` prefix (e.g. `c_mode_strs`, `c_chamfer_mode_strs`).
- **Functions / methods**: snake_case (e.g. `add_new_node`, `get_node_exact`, `try_get_node_idx_snap`).
- **Private methods**: snake_case with trailing underscore (e.g. `update_axis_snap_anno_`).
- **Type aliases**: snake_case with suffix by role, e.g. `*_ptr` for handles (`AIS_Shape_ptr`, `Shp_ptr`), `*_rslt` for result types (`Shp_rslt`). Typedefs like `ScreenCoords` are PascalCase.
- **Macros**: UPPER_SNAKE_CASE (e.g. `EZY_ASSERT`, `EZY_ASSERT_MSG`, `DBG_MSG`).

## Formatting

### Line endings

- Use **LF** (`\n`) for all EzyCad-owned text files: `src/`, `tests/`, `scripts/`, `cmake/`, `docs/`, `agents/`, and other project paths listed in **`.gitattributes`**.
- Do **not** mix CRLF and LF in the same file; mixed endings trigger editor warnings and noisy diffs.
- **`third_party/`** is vendored upstream code and is not normalized by this rule.
- On Windows, configure your editor to **LF** for C++ (Visual Studio: *File → Advanced Save Options*, or normalize when prompted). In this repo, run `git config core.autocrlf false` so Git does not reintroduce CRLF on checkout. Git applies the rules in `.gitattributes` on commit.

### clang-format

Run **`scripts/format-src.ps1`** (or `clang-format -i` on individual files) before committing C++ changes. **`.clang-format`** at the repo root is the source of truth; the settings below match that file. Anything not listed inherits **`BasedOnStyle: LLVM`** (e.g. spaces not tabs, short loops not collapsed to one line).

- **`BasedOnStyle: LLVM`** — Start from the LLVM preset; only the options below override it.
- **`IndentWidth: 2`** / **`TabWidth: 2`** — Indent with **2 spaces**; do not use tab characters.
- **`ColumnLimit: 128`** — Prefer wrapping before column 128.
- **`BreakBeforeBraces: Allman`** — Opening `{` on the **next line** for classes, structs, functions, control statements, namespaces, and enums.
- **`BreakConstructorInitializers: BeforeComma`** — Constructor initializer lists break **before** each comma (comma starts the continuation line).
- **`AccessModifierOffset: -1`** — Access specifiers are outdented one space: `` ` public:` ``, `` ` private:` ``, `` ` protected:` `` (one leading space).
- **`PointerAlignment: Left`** — Attach `*` / `&` to the type: `int* p`, `const Shp& shp`.
- **`AlignConsecutiveDeclarations: true`** — Align types and names across consecutive declarations in the same block when it helps readability.
- **`AlignConsecutiveAssignments: true`** — Align `=` across consecutive assignments in the same block when it helps readability.
- **`AllowShortIfStatementsOnASingleLine: false`** — Do not put an entire `if` (condition + body) on one line; condition and statement stay on separate lines. You may still omit braces for a single-statement body.
- **`AllowShortLambdasOnASingleLine: Inline`** — Very short **inline** lambdas may stay on one line; longer lambdas break across lines.
- **`IndentCaseLabels: false`** — `case` / `default` labels align with the surrounding `switch`, not extra-indented under it.
- **`SortIncludes: false`** — clang-format does **not** reorder `#include` lines. Follow **Headers** above manually (own header first in `.cpp`, project `"..."` before `<...>`).

Use **`// clang-format off`** / **`// clang-format on`** only where layout must be preserved (e.g. macro-like tables). Prefer formatting the file normally.

**Not enforced by clang-format** (project convention): brace-initialize members (`bool ok {false};`); declare locals close to first use; omit braces on single-statement `if`/`for`/`while` bodies when clear.

## Documentation

User-facing Markdown (now in `docs/`: `usage.md`, `usage-*.md`, `scripting.md`, `building-occt.md` etc., plus Read the Docs, images, in-app help URLs) is covered in **[ezycad_doc_style.md](ezycad_doc_style.md)**. This file is for **`src/` C++ only**.

## Versioning and releases

- **Product version** lives in **`src/version.h`**: `EZYCAD_VERSION_MAJOR`, `EZYCAD_VERSION_MINOR`, `EZYCAD_VERSION_PATCH`, and `EZYCAD_VERSION_STRING`. Follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html) (MAJOR.MINOR.PATCH). The header is hand-maintained so it stays valid regardless of build system (CMake or otherwise).
- **Release notes** live in **`CHANGELOG.md`** at the repo root, using [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) sections (`Added`, `Changed`, `Fixed`, etc.). Add notable work under **`[Unreleased]`** as you go; when you cut a release, rename that section to **`[x.y.z] - YYYY-MM-DD`**, bump **`version.h`** to match, and create a git tag **`vx.y.z`** consistent with `EZYCAD_VERSION_STRING`.
- **Do not confuse** the product version with **`k_settings_version`** in `gui_settings.cpp` (and `version` in saved settings JSON): that value is the **settings file format** version, not the application version.

## Code organization

- **Reader-first order** (`.cpp`): Put **public API and high-level workflow** at the top of the file (constructors, main entry points, orchestration). Put **lower-level details** below so the first screen shows what the module does before how it does it.
- **Helper functions** (`.cpp`): Prefer **file-local static helpers** in an anonymous namespace at the **bottom** of the implementing `.cpp` file. Forward-declare them near the top (or above their first use) when needed. Avoid large helper blocks at the top of the file; the goal is high-level code first, helpers last for readability.
- **PIMPL** (`class Foo; class Foo::Impl`): Use when hiding implementation details or when you want to swap implementations (e.g. `Sketch_nodes`, `Sketch_op_recorder`). Keep the public surface in the header; put data members, private record types, and apply/clone logic in `Impl` inside the `.cpp`.
- **Templates**: Prefer putting template implementations in `.inl` files included from the header (e.g. `utl_types.inl`, `utl.inl`).
- **OCCT handles**: Prefer `opencascade::handle<T>` and project `*_ptr` aliases from `utl_types.h` (e.g. `AIS_Shape_ptr`, `Shp_ptr`). Avoid the OCCT `Handle(T)` macro in new/touched code -- clang-format mishandles it with `PointerAlignment: Left` (e.g. `Handle(Foo) &`). See [agents/conventions/occt-handles.md](../agents/conventions/occt-handles.md).
- **OCCT desktop vs WASM**: Until WASM runs correctly on OCCT 8, desktop is OCCT **8** and recommended WASM is **7.9.3**. Prefer APIs available on both; for `Standard_Failure` use `GetMessageString()`, not `what()`. See [agents/conventions/occt-wasm-dual-version.md](../agents/conventions/occt-wasm-dual-version.md).
- **Result/error handling**: See **Fail fast** below. Use `Result<T>` and `Status` from `utl.h`, `CHK_RET(...)` for early return on failure.
- **Assertions**: See **Fail fast** below. Use `EZY_ASSERT` and `EZY_ASSERT_MSG` from `utl_dbg.h`; use `DBG_MSG` for debug logging.

## Don't Repeat Yourself (DRY)

- DRY is important, but it is guidance, not a strict rule.
- **Reader-first pairing**: Pragmatic DRY matches **Reader-first order** in **Code organization** above. Prefer a self-contained workflow or UI pane you can read top-to-bottom over distant abstractions whose only win is deleting a few repeated lines.
- Avoid hasty abstractions: do not generalize too early before patterns are stable.
- Too much DRY can increase coupling by forcing unrelated code through one shared abstraction.
- Prefer readability over clever reuse when repetition is small and explicit code is clearer.
- **When duplication is acceptable**: a few lines repeated across nearby UI or glue code can beat a shared helper if call sites are likely to diverge (different tooltips, widths, or disabled logic) or if extracting would scatter one screen across many symbols. Prefer **locality**: keep a small block self-contained so a reader does not jump to understand one pane.
- **Rule of thumb**: extract when the behavior is **the same and stable** (or when a bug fix must touch N copies); wait when the pattern is still moving. If you extract, prefer a **file-local static helper at the bottom of the `.cpp`** (see **Code organization**) over a generic framework.
- Context matters: stronger DRY is often good in monolith/shared-library code; some duplication can be healthier in fast-changing or separated systems.
- Balance DRY with KISS, YAGNI, and overall cognitive load.

## Fail fast

EzyCad uses **fail fast**: detect problems as early as possible. Do not hide programmer mistakes with silent fallbacks (e.g. substituting empty defaults when a required dependency is missing) unless that behavior is an explicit, documented product decision.

Separate **who is responsible** for the fault:

### Programmer errors (use asserts)

Use **`EZY_ASSERT`** / **`EZY_ASSERT_MSG`** (`utl_dbg.h`) for conditions that must hold if the code and call patterns are correct:

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

| Kind of problem                                   | Mechanism                                               |
| ------------------------------------------------- | ------------------------------------------------------- |
| Bug, contract violation, impossible branch        | `EZY_ASSERT` / `EZY_ASSERT_MSG`                         |
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
- For AI tools: the same rule is summarized in `agents/conventions/ascii-source.md`.

## C++ usage

- Prefer **`enum class`** for enumerations.
- Use **`std::optional`** where a value may be absent.
- Use **`const`** and **`const&`** for parameters and accessors where appropriate.
- Prefer **glm vector types** (`glm::vec2/vec3/vec4`, `glm::dvec*`) for geometric/color-like grouped values
  (positions, directions, extents, RGB/RGBA colors) instead of raw C arrays when storing or passing data in project
  code. Use raw arrays only at API boundaries that require `float*`/`double*` (e.g. some ImGui or OCCT calls), and
  convert at the boundary.
- Keep consistent with existing file style (e.g. include order, brace placement) when editing.
