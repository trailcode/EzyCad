# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **NumPad 5:** snap the 3D view to the nearest orthographic world-axis orientation (top/bottom/front/back/left/right) with roll reset; keeps eye-target distance.
- **View roll:** **Shift+NumPad 4** and **Shift+NumPad 6** roll the 3D view (Blender-style). **Settings** has **View roll step** (degrees per key press; default 45, stored as `gui.view_roll_step_deg`).
- Help > About: reads bundled `res/about.md` via [imgui_markdown](https://github.com/enkisoftware/imgui_markdown), shows version text, splash image (`res/AI-gen-splashscreen_05_01_2026_512.png`), and clickable links; assets are copied next to the executable (and preloaded for Emscripten).
- **Sketch length dimensions (Dimension tool):** dimensions are stored as an unordered pair of sketch nodes (not attached to a single edge). They can be selected in sketch mode and removed with **Delete**.
- **Project JSON:** root-level `ezyFormat` (current value `2`) on save. Sketches serialize `length_dimensions` as dense node-index pairs alongside linear edges as `[a, b, mid]` only.
- **Legacy load:** older sketch JSON that stored a per-edge dimension flag on indexed edges (`[a, b, mid, dim]`) or on legacy coordinate edges (`[pt_a, pt_b, dim]`) migrates those flags into `length_dimensions` when loading.

### Fixed

- Ship `res/default.ezy` (empty sketch template) so native copy steps and Emscripten `--preload-file` resolve the path; startup loading could already fall back if the file was absent.

### Changed

- **View roll** default step is **45** degrees (was 15).
- Window title shows the current project file name (e.g. after Open or Save), or `untitled` when there is no path yet; **File > New** clears the name so the title matches an empty document.
- **Dimension tool behavior:** click a straight edge to toggle a length dimension between its endpoints, or click two nodes to toggle a dimension between them; clicks away from nodes and edges do not create spurious dimensions. Node picks take precedence over edge picks when both could apply; moving the mouse updates node snap feedback in this mode.
- **Documentation:** `usage-sketch.md` and `usage.md` describe the tool as the **Dimension tool** and document the node-pair model.

## [0.1.0] - 2026-04-17

### Added

- Initial version tracking: `src/version.h` and this changelog.

[Unreleased]: https://github.com/trailcode/EzyCad/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/trailcode/EzyCad/releases/tag/v0.1.0
