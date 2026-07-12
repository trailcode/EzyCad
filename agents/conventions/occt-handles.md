# OCCT handles vs `Handle()` macro

Prefer the pattern in [`src/utl_types.h`](../../src/utl_types.h):

```cpp
using AIS_Shape_ptr = opencascade::handle<AIS_Shape>;
```

Use those `*_ptr` aliases (or `opencascade::handle<T>` inline) in new and touched code.

## Why not `Handle(T)`?

OCCT's `Handle(T)` macro is valid and expands to `opencascade::handle<T>`, but **clang-format does not treat it as a type**. With `PointerAlignment: Left`, format runs produce awkward spacing such as:

```cpp
const Handle(Geom_TrimmedCurve) & curve;  // bad (formatter)
const Geom_TrimmedCurve_ptr& curve;       // good
```

Do **not** paper over this with `// clang-format off` for routine handle declarations.

## Agent rules

1. For a type used as a handle in EzyCad, add or reuse a `using TypeName_ptr = opencascade::handle<TypeName>;` in `utl_types.h` (forward-declare the class there if needed, matching existing entries).
2. Prefer `TypeName_ptr` in signatures, locals, and members.
3. `DownCast`: `TypeName_ptr::DownCast(obj)` (or `opencascade::handle<TypeName>::DownCast(obj)`), not `Handle(TypeName)::DownCast(obj)`.
4. When editing a function that already uses `Handle(...)`, convert that local scope to `*_ptr` / `opencascade::handle` if the change is small; do not mass-rewrite unrelated files.

Style summary: [docs/ezycad_code_style.md](../../docs/ezycad_code_style.md) (OCCT handles).
