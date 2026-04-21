# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Help > About: in-app dialog with the app version, a short product description, and a link to the GitHub repository (replaces opening the README in the browser from that menu item).
- **Sketch length dimensions (Dimension tool):** dimensions are stored as an unordered pair of sketch nodes (not attached to a single edge). They can be selected in sketch mode and removed with **Delete**.
- **Project JSON:** root-level `ezyFormat` (current value `2`) on save. Sketches serialize `length_dimensions` as dense node-index pairs alongside linear edges as `[a, b, mid]` only.
- **Legacy load:** older sketch JSON that stored a per-edge dimension flag on indexed edges (`[a, b, mid, dim]`) or on legacy coordinate edges (`[pt_a, pt_b, dim]`) migrates those flags into `length_dimensions` when loading.

### Changed

- Window title shows the current project file name (e.g. after Open or Save), or `untitled` when there is no path yet; **File > New** clears the name so the title matches an empty document.
- **Dimension tool behavior:** click a straight edge to toggle a length dimension between its endpoints, or click two nodes to toggle a dimension between them; clicks away from nodes and edges do not create spurious dimensions. Node picks take precedence over edge picks when both could apply; moving the mouse updates node snap feedback in this mode.
- **Documentation:** `usage-sketch.md` and `usage.md` describe the tool as the **Dimension tool** and document the node-pair model.

## [0.1.0] - 2026-04-17

### Added

- Initial version tracking: `src/version.h` and this changelog.

[Unreleased]: https://github.com/trailcode/EzyCad/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/trailcode/EzyCad/releases/tag/v0.1.0
