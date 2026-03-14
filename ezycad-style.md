# EzyCad coding style

Use this style when editing or adding C/C++ code in the EzyCad project (files under `src/`, not third_party or build output).

## Headers

- Use **`#pragma once`** for include guards (no `#ifndef`/`#define`/`#endif`).
- **Include order**: Put the **closest / most direct** dependencies first.
  - In a **`.cpp`** file: include **this file's own header first** (e.g. in `sketch_nodes.cpp`, the first line is `#include "sketch_nodes.h"`). Then other project headers it needs, then external/system headers (`<...>`). Use a blank line before the `<...>` block when you have both.
  - In a **`.h`** file: include the project headers this file directly depends on first (e.g. `sketch_nodes.h` has `#include "types.h"` before `<gp_Pln.hxx>`; `shp_operation.h` has `"utl_result.h"`, `"shp.h"`, `"types.h"` first). Then external/system headers. So **project (`"..."`) before external (`<...>`)**, with the most relevant project header at the very top when there is one.
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
- **Short control flow**: Single-line `if`/`for` without braces is acceptable when the body is a single statement; use braces for multi-line or nested bodies.
- Use **`// clang-format off`** / **`// clang-format on`** only where layout must be preserved (e.g. macro-like blocks, tables). Prefer running clang-format; it is the source of truth for formatting.

## Code organization

- **Templates**: Prefer putting template implementations in `.inl` files included from the header (e.g. `types.inl`, `utl_result.inl`).
- **OCCT handles**: Use `opencascade::handle<T>` and project aliases (e.g. `AIS_Shape_ptr`, `Shp_ptr`).
- **Result/error handling**: Use the project `Result<T>` and `Status` types from `utl_result.h` (e.g. `Status::ok()`, `Status::user_error("...")`, `CHK_RET(...)` where used).
- **Assertions / debug**: Use project macros `EZY_ASSERT` and `EZY_ASSERT_MSG` from `dbg.h`; use `DBG_MSG` for debug logging.

## Comments

- Use `//` for line comments.
- Document non-obvious parameters or lifetime requirements (e.g. `// \`view\` must exist for the lifetime of this object`).
- Prefer brief inline comments for non-obvious logic; keep comments in sync with code.

## C++ usage

- Prefer **`enum class`** for enumerations.
- Use **`std::optional`** where a value may be absent.
- Use **`const`** and **`const&`** for parameters and accessors where appropriate.
- Keep consistent with existing file style (e.g. include order, brace placement) when editing.
