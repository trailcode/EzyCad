---
status: implemented
topic: sketch-mode-shape-faint
helps:
  - shp-origin-orientation
  - cross-section-tool
  - sketch-from-shape
---

# Sketch-mode faint shapes

**Load only when** the prompt is about dimming / ghosting / wireframing 3D shapes while in sketch mode. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

## Status

**Implemented** (prototype): Settings **Shapes in sketch mode** = Off / Ghost (default) / Wire; Ghost opacity; `Occt_view::sync_sketch_shape_faint_style` on mode change and new shapes.

## Why

In sketch modes, solids used to be fully hidden. Faint display keeps context for shape origin work, cross-section, and sketching near geometry without **Hide all**.

## Behavior

| Style     | Effect in `is_sketch_mode`                                 |
| --------- | ---------------------------------------------------------- |
| Off (0)   | Hide shapes (legacy)                                       |
| Ghost (1) | Semi-transparent shaded (`gui.sketch_shape_faint_opacity`) |
| Wire (2)  | Wireframe                                                  |

Leave sketch mode: clear `Shp::set_sketch_faint`, restore user `get_disp_mode()`. **Hide all** still hides everything.

## Code

- [`shp.h`](../../src/shp.h) — `set_sketch_faint` / `effective_disp_mode_`
- [`gui_occt_view.cpp`](../../src/gui_occt_view.cpp) — `sync_sketch_shape_faint_style`, `on_mode`, `add_shp_`
- [`gui_settings.cpp`](../../src/gui_settings.cpp) / [`gui.h`](../../src/gui.h) — keys `sketch_shape_faint_style`, `sketch_shape_faint_opacity`
- Docs: `docs/usage-settings.md`, `docs/usage-sketch.md`, `src/doc/gui.md`, `src/doc/shape.md`, `CHANGELOG.md`
