---
status: planning
topic: sketch-mode-shape-faint
helps:
  - shp-origin-orientation
  - cross-section-tool
  - sketch-from-shape
---

# Sketch-mode faint shapes

**Load only when** the prompt is about dimming / ghosting / wireframing 3D shapes while in sketch mode. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

## Why

In sketch modes, solids stay fully shaded and compete with sketch edges/faces. A **faint** shape representation (alpha and/or wireframe) keeps context for:

- Placing / seeing a [shape origin triad](shp-origin-orientation.md)
- [Cross-section](cross-section-tool.md) relative to the solid
- Sketching near existing geometry without “Hide all”

Today: per-shape Shaded/WireFrame toggle and “Hide all” in Shape List ([`gui.cpp`](../../src/gui.cpp)); no automatic sketch-mode dimming. Viewer already uses `Graphic3d_RTM_BLEND_UNORDERED` ([`gui_occt_view.cpp`](../../src/gui_occt_view.cpp)). Non-current sketches use `Edge_style::Background` — analogous idea for shapes.

## Goal

When `is_sketch_mode(mode)` (and optionally related modes like polar dup):

- Document shapes render **faint**: transparency and/or forced wireframe.
- Leaving sketch mode restores each shape’s prior display (shaded/wire + opacity).
- User override: keep “Hide all”; optional Settings slider for sketch-mode shape opacity / wireframe-vs-ghost.

## Design lean (prototype)

| Choice    | Default                                                                                                                                                      |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Trigger   | Enter/leave via `Occt_view::on_mode` / `GUI::set_mode` when `is_sketch_mode` flips                                                                           |
| Look      | Semi-transparent shaded **or** wireframe (try both; Settings: Ghost / Wire / Off)                                                                            |
| State     | Cache per-`Shp` previous `AIS_DisplayMode` + transparency; restore on exit                                                                                   |
| Selection | Shapes still pickable only if needed; default: reduced interference with sketch picks (may disable shape selection in sketch tools — match current behavior) |

## Likely touch points

- [`mode.h`](../../src/mode.h) / `is_sketch_mode`
- [`Occt_view::on_mode`](../../src/gui_occt_view.h) or GUI mode switch
- [`Shp::set_disp_mode`](../../src/shp.h) + `AIS_Shape::SetTransparency`
- Settings key under `gui.*` + [`gui_settings.cpp`](../../src/gui_settings.cpp)
- Docs: [`src/doc/gui.md`](../../src/doc/gui.md), [`docs/usage-sketch.md`](../../docs/usage-sketch.md) when shipped

## Acceptance

- [ ] Enter sketch mode → shapes faint; leave → restore previous look.
- [ ] New shapes added while in sketch mode also get faint style.
- [ ] Hide-all still works; undo/visibility not broken.
- [ ] Setting to disable or choose Ghost vs Wire.

## Out of scope

Changing sketch edge Background styling; section/sketch-from-shape logic.
