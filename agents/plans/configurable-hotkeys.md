---
status: implemented
topic: configurable-hotkeys
depends_on: null
blocks:
  - hold-key-duplicate-translate
---

# Configurable hotkeys

**Load only when** the prompt is about remappable shortcuts, keybindings, `gui.hotkeys`, or freeing `D` for duplicate-on-key. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

## Why

Hold-`D` duplicate conflicts with hardcoded `D` -> Dimension. A Settings-backed action map lets users remap Dimension (and other tools) without one-off chord hacks.

## Shipped

- `Gui_hotkeys` / `Gui_action` in [`src/gui_hotkeys.h`](../../src/gui_hotkeys.h): chord map, conflict reject, `format_chord` / `parse_chord`.
- `GUI::on_key` dispatches remappable actions via lookup; Esc/Tab/Enter/digits/view and Delete/Backspace stay fixed; Ctrl+Shift+Z is a fixed redo alias.
- Settings **Keyboard shortcuts** + persist `gui.hotkeys`; Defaults / missing keys merge to built-ins.
- Toolbar tooltips for remappable modes and boolean commands follow the binding table.
- Defaults cover shape tools, all sketch toolbar tools, polar duplicate, cross-section, and Cut/Fuse/Common.

## Follow-up (not done)

**Hold-key duplicate translate:** new action (e.g. `edit.duplicate_translate`), default unbound or `Ctrl+D`; needs PRESS/RELEASE session + clone + `Shp_move`-style preview. Enabled once users can free `D` or pick another chord via this system.

**Later:** custom multi-chord sequences; per-mode binding contexts; WASM #93 parity; warn when remapping onto fixed keys (digits/Esc/Tab).
