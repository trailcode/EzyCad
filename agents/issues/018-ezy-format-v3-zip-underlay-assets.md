# .ezy format v3: zip container + underlay asset refs

**Status:** implemented on `main`

---

## Summary

Upgrade `.ezy` from monolithic JSON to a self-contained ZIP (`ezyFormat: 3`):

- `manifest.json` — sketches, shapes, view (BRep still inline)
- `assets/<hash>.rgba` — deduplicated underlay RGBA8 blobs
- Sketch underlay JSON uses `"asset"` ref instead of `"rgba_b64"`

Legacy plain-JSON `.ezy` files continue to load (`rgba_b64` migrated into the session asset store on read).

## Key files

- `src/ezy_asset_store.h/.cpp` — content-hash registry, shared pixel blobs
- `src/ezy_io.h/.cpp` — zip pack/unpack, format sniff, base64 for Wasm startup storage
- `src/sketch_underlay.*`, `src/sketch_json.cpp`, `src/occt_view.*`, `src/gui.cpp`, `src/utl_settings.cpp`

## Out of scope (phase 2)

- External BRep blobs (`shapes[].geom`, `originating_face`)
- Asset GC across long sessions
