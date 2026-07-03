# Branch `Trailcode/refactor1`: sketch nodes refactor, dimension settings, OCCT wasm build

**Opened on GitHub:** https://github.com/trailcode/EzyCad/issues/113

**Branch:** `Trailcode/refactor1` (7 commits ahead of `main` at open time)

**Labels:** `enhancement`, `documentation`

---

## Title (GitHub)

Sketch nodes PIMPL refactor, global dimension settings, OCCT 8 wasm build tooling

## Body (GitHub)

### Summary

Tracking branch **`Trailcode/refactor1`**: refactor sketch-node internals, add persisted **Settings → Sketch → Dimensions** controls (line width, arrows, label rendering, visibility), fix permanent node annotation scale, and add **OCCT V8.0.0 WebAssembly** build scripts plus **`docs/building-occt.md`**.

### Scope (implemented on branch)

**Sketch nodes (`src/sketch_nodes.cpp`, `src/sketch_nodes.h`):**

- PIMPL-style refactor (`Sketch_nodes` implementation detail moved behind `m_impl`).
- Snap / axis-guide behavior preserved; cleaner separation from `Sketch`.

**Dimension style (`src/geom.h`, `src/geom.cpp`, `src/gui.*`, `src/sketch.cpp`, `src/occt_view.*`, `src/shp_extrude.*`):**

- `Length_dimension_style` bundles global dimension display settings.
- `apply_length_dimension_style()` applies `Prs3d_DimensionAspect` + AIS Z-layer for labels.
- **Settings → Sketch → Dimensions** (nested): line width, arrow size/color, text scale, **label rendering** (0–5; default **Z-layer Topmost** to avoid grid ghosting), placement, arrow style/orientation, **show sketch dimensions**.
- `Occt_view::refresh_all_length_dimensions()` rebuilds dims when settings change.
- Removed non-functional flyout / extension-line settings after OCCT limitations.

**Other:**

- Permanent node annotation scale fix (`gui.permanent_node_anno_scale`).
- **`scripts/build-occt-v8-wasm.ps1`** + **`scripts/build-occt-v8-wasm.cmd`** — download FreeType, build OCCT `V8_0_0` static + GLES2 for Emscripten.
- **`docs/building-occt.md`** — Windows prebuilts, wasm build, troubleshooting, wrapper-script patterns.
- **`usage-settings.md`** updated for new dimension controls.

### Out of scope / follow-ups

- [ ] Verify **EzyCad wasm** links against OCCT 8 install produced by the new script (experimental; README still references 7.9.x manual path).
- [ ] Review commits **`Debug code`** / **`WIP`** — drop or gate debug-only changes before merge.
- [ ] Desktop upgrade to **OCCT 8.0.0** prebuilts (separate from wasm); retest grid + dimension labels after OCCT 8 grid shader changes.
- [ ] Consider typed enum for `edge_dim_text_render_mode` instead of magic `int` 0–5.

### Test plan

- [ ] **Settings → Sketch → Dimensions:** change each control; confirm live refresh on sketch length dimensions and extrude preview dims.
- [ ] **Label rendering:** try **Z-layer Top** / **Topmost** (defaults); confirm no grid bleed-through on labels.
- [ ] **Show sketch dimensions** global toggle; per-sketch list overrides still work.
- [ ] Sketch snap: dimension-tool hover, cross-sketch snap, add-node on edge interior (`Sketch_test` if available).
- [ ] **Permanent node annotation size** slider visible effect on sketch nodes.
- [ ] Desktop Release build; open/save settings JSON (`edge_dim_*` keys).
- [ ] (Optional) `.\scripts\build-occt-v8-wasm.ps1` completes; `emcmake` EzyCad with printed `OpenCASCADE_DIR`.

### Links

- Branch: `Trailcode/refactor1`
- Doc: `docs/building-occt.md`
