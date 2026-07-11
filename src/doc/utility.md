# Utility module (`utl_*`)

Developer reference for shared C++ helpers under the `utl_` prefix. Umbrella header: [`utl.h`](../utl.h).

Maintainers: update this file when utility APIs, result types, I/O formats, or settings paths change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)).

## Purpose

`utl_*` files hold **cross-cutting utilities** used by sketch, shape, GUI, and scripting code: error/results, geometry, OCCT helpers, project I/O, settings persistence, logging, and shared typedefs. They are not a single class; include the smallest header that matches your dependency.

Typical uses:

- Return `Status` / `Result<T>` from operations (`CHK_RET` macro).
- Plane/point/wire helpers for sketch and extrude (`utl_geom`).
- Pack/unpack `.ezy` v3 zip projects (`utl_io` + `Ezy_asset_store`).
- Load/save `ezycad_settings.json` (`settings` namespace in `utl_settings`).
- OCCT handle typedefs and `ScreenCoords` (`utl_types`).

## Module map

| File                                                                           | Responsibility                                                                             |
| ------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------ |
| [`utl.h`](../utl.h) / [`utl.cpp`](../utl.cpp)                                  | `Status`, `Result<T>`, `CHK_RET`, `clear_all`, textures, image decode, name uniquification |
| [`utl_types.h`](../utl_types.h)                                                | OCCT/AIS handle typedefs, `ScreenCoords`, `Export_format`, `DECL_PTR`, `SafeType`          |
| [`utl_geom.h`](../utl_geom.h) / [`.cpp`](../utl_geom.cpp)                      | 2D/3D geometry, wires, dimensions, Boost polygon tests, plane projection                   |
| [`utl_geom_boost.inl`](../utl_geom_boost.inl)                                  | `ezy_geom` Boost.Geometry aliases                                                          |
| [`utl_occt.h`](../utl_occt.h) / [`.cpp`](../utl_occt.cpp)                      | `TopAbs` name table, `try_make_solid`                                                      |
| [`utl_json.h`](../utl_json.h) / [`.cpp`](../utl_json.cpp)                      | JSON serializers for `gp_Pnt`, `gp_Pln`, etc.                                              |
| [`utl_io.h`](../utl_io.h) / [`.cpp`](../utl_io.cpp)                            | `.ezy` zip v3 pack/unpack, format sniff, base64                                            |
| [`utl_asset_store.h`](../utl_asset_store.h) / [`.cpp`](../utl_asset_store.cpp) | Content-addressed RGBA blobs for sketch underlay assets                                    |
| [`utl_settings.h`](../utl_settings.h) / [`.cpp`](../utl_settings.cpp)          | User settings file paths, startup project blob I/O                                         |
| [`utl_ply_io.h`](../utl_ply_io.h) / [`.cpp`](../utl_ply_io.cpp)                | PLY import/export for mesh shapes                                                          |
| [`utl_log.h`](../utl_log.h) / [`.cpp`](../utl_log.cpp)                         | `Log_strm` redirecting stdout/stderr to `GUI::log_message`                                 |
| [`utl_dbg.h`](../utl_dbg.h)                                                    | `EZY_ASSERT`, `DBG_MSG`, debug break macros                                                |

CMake IDE group: `src\utl` (pattern `^utl(_|\.)`).

## Core types (`utl.h`)

### `Status` and `Result<T>`

| Type                               | Use                                                        |
| ---------------------------------- | ---------------------------------------------------------- |
| `Status::ok()` / `user_error(msg)` | Void operations; user-facing message string                |
| `Result<T>`                        | Value or error (`Shp_rslt` = `Result<Shp_ptr>` in `shp.h`) |
| `CHK_RET(expr)`                    | Early-return if sub-call returns non-ok `Status`           |

### General helpers

| API                                      | Purpose                                          |
| ---------------------------------------- | ------------------------------------------------ |
| `clear_all(...)`                         | Reset optional/containers/arithmetic in one call |
| `unique_sequential_name(base, existing)` | `Name`, `Name.001`, ... for sketches/shapes      |
| `load_texture(path)`                     | Toolbar icon loading                             |
| `decode_image_bytes(bytes)`              | stb_image -> RGBA for underlay import            |
| `safe_cstr_copy`                         | ImGui fixed-buffer copies (MSVC-safe)            |

## Geometry (`utl_geom`)

Large module; grouped by concern:

| Area              | Examples                                                                                                                                                                                                                                                                            |
| ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Plane / 2D        | `to_2d`, `to_3d`, `xy_plane`, `sketch_reference_plane`, `Plane_side`                                                                                                                                                                                                                |
| Profile wires     | `make_square_wire`, `make_circle_wire`, `make_slot_wire`, `create_wire_box`                                                                                                                                                                                                         |
| Sketch dimensions | `Length_dimension_style`, `create_distance_annotation`, `apply_length_dimension_style`                                                                                                                                                                                              |
| Analysis          | `to_boost` (polygon), `to_boost_ls` (edge `linestring_2d`), `get_shape_bbox_center`, `plane_from_face`, `side_of_plane`                                                                                                                                                             |
| Tests / debug     | `to_wkt_string` (linestring / ring / polygon), `ezy_geom::area`, `is_valid`; Geometry Watch in `scripts/ezycad_graphical_debugging.xml` (`ring_2d` inherits vector as Ring; `linestring_2d` uses named `points` as Linestring). Re-select the XML path in Options after editing it. |

Includes [`utl_geom_boost.inl`](../utl_geom_boost.inl) for `ezy_geom` polygon / linestring types used in tests and face validation.

## Project I/O (`utl_io` + `Ezy_asset_store`)

### `.ezy` v3 zip layout

| Path in archive    | Content                                           |
| ------------------ | ------------------------------------------------- |
| `manifest.json`    | Document JSON (sketches, shapes, view state)      |
| `assets/<id>.rgba` | Raw RGBA pixels for underlay `"asset"` references |

| Function                       | Role                                                            |
| ------------------------------ | --------------------------------------------------------------- |
| `is_ezy_zip` / `is_ezy_json`   | Sniff loaded bytes                                              |
| `unpack_ezy(bytes)`            | -> manifest + asset map                                         |
| `pack_ezy(manifest, store)`    | Build zip from manifest + store entries referenced by underlays |
| `ezy_base64_encode` / `decode` | Emscripten startup project in localStorage                      |

`Ezy_asset_store` deduplicates RGBA by FNV-1a id (`register_rgba`, `get`, `import_asset`). Owned on `Occt_view` for the session.

## Settings (`settings` namespace)

| API                                      | Role                                          |
| ---------------------------------------- | --------------------------------------------- |
| `load_defaults()`                        | Ship defaults from `res/ezycad_settings.json` |
| `load_with_defaults()`                   | User file or defaults + save                  |
| `save(content)`                          | Native file or Wasm localStorage              |
| `user_config_directory()`                | `%APPDATA%/EzyCad`, etc.                      |
| `user_settings_json_path()`              | Persisted GUI/OCCT settings                   |
| `load/save/clear_user_startup_project()` | Optional startup `.ezy` blob                  |

`GUI::load_occt_view_settings_` / `save_occt_view_settings` parse and emit JSON keys documented in [`docs/usage-settings.md`](../../docs/usage-settings.md).

## OCCT helpers (`utl_occt`)

| API                        | Role                                                     |
| -------------------------- | -------------------------------------------------------- |
| `c_names_TopAbs_ShapeEnum` | String names parallel to OCCT enum (selection filter UI) |
| `try_make_solid(shape)`    | Wrap closed shell as solid when possible                 |

## JSON geom (`utl_json`)

Symmetric `to_json` / `from_json_*` for `gp_Pnt2d`, `gp_Pnt`, `gp_Dir`, `gp_Pln` used in sketch/project serialization.

## PLY (`utl_ply_io`)

| API                                   | Role                                                |
| ------------------------------------- | --------------------------------------------------- |
| `import_ply_shape(bytes, out)`        | ASCII or binary_little_endian PLY -> `TopoDS_Shape` |
| `export_ply_binary_file(shape, path)` | Mesh export (mesh shape first)                      |

## Logging and debug

| Component                       | Role                                                    |
| ------------------------------- | ------------------------------------------------------- |
| `Log_strm`                      | Installed on cout/cerr in `GUI::setup_log_redirection_` |
| `EZY_ASSERT` / `EZY_ASSERT_MSG` | Debug-break assertions (`utl_dbg.h`)                    |
| `DBG_MSG`                       | Verbose debug logging macro                             |

## Typical developer usage

### Propagate operation errors

```cpp
Status Shp_fuse::selected_fuse() {
  view().push_undo_snapshot();
  CHK_RET(ensure_operation_multi_shps_());
  // ...
  return Status::ok();
}
```

### Pack a project with underlay assets

```cpp
const std::string manifest = view.to_json();
std::vector<uint8_t> ezy = pack_ezy(manifest, view.asset_store());
```

### Project plane point

```cpp
gp_Pnt2d uv = to_2d(sketch.get_plane(), pick_3d);
gp_Pnt back = to_3d(sketch.get_plane(), uv);
```

## Dependencies and layering

| Layer | May include                                                                |
| ----- | -------------------------------------------------------------------------- |
| Low   | `utl_dbg`, `utl_types`                                                     |
| Mid   | `utl`, `utl_json`, `utl_occt`, `utl_io`, `utl_asset_store`, `utl_settings` |
| Heavy | `utl_geom` (OCCT + Boost + glm), `utl_ply_io`                              |

Avoid circular includes: `utl_types.h` pulls sketch AIS typedefs via `sketch_ais.h`; geometry code should not include GUI headers.

## Testing

| Item                  | Location                                                      |
| --------------------- | ------------------------------------------------------------- |
| Geometry / polygon    | `tests/sketch_topo_tests.cpp` (`ezy_geom::`, `to_wkt_string`) |
| `.ezy` zip / underlay | `tests/sketch_json_tests.cpp` (`pack_ezy`, `is_ezy_zip`)      |
| Settings              | Manual; paths vary by platform                                |

## Related code outside `src/utl*`

| Location                                        | Role                                             |
| ----------------------------------------------- | ------------------------------------------------ |
| [`gui.cpp`](../gui.cpp)                         | `pack_ezy` / `unpack_ezy` for file open/save     |
| [`sketch_underlay.cpp`](../sketch_underlay.cpp) | `Ezy_asset_store` for underlay pixels            |
| [`sketch_json.cpp`](../sketch_json.cpp)         | Manifest JSON + asset references                 |
| [`gui_settings.cpp`](../gui_settings.cpp)       | Parses `gui.*` / `occt_view.*` keys into members |
