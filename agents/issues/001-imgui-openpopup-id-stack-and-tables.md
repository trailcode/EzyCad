# [Draft] ImGui: document pattern for OpenPopup vs BeginPopup outside tables

**Paste into GitHub as a new issue.** Suggested labels: `documentation`, `imgui`, `low priority`

---

## Title

Document ImGui popup ID stack: avoid OpenPopup inside `BeginTable` unless BeginPopup matches scope

## Body

### Summary

Dear ImGui resolves `OpenPopup("id")` / `BeginPopup("id")` using the current **ID stack** (see [imgui#331](https://github.com/ocornut/imgui/issues/331)). Calling `OpenPopup` from inside a **table cell** while calling `BeginPopup` **after** `EndTable()` produces **different hashed popup IDs**, so the popup never opens.

### Context in EzyCad

We hit this when adding UI next to **Settings → 3D view navigation → View roll step** (slider inside `BeginTable`). The fix was to **defer** `OpenPopup` until after `ImGui::EndTable()`, in the same ID scope as `BeginPopup`.

**View roll (for users):** **Shift+NumPad 4** and **Shift+NumPad 6** roll the 3D view around the screen axis (Blender-style). The step in degrees is **Settings → 3D view navigation → View roll step** (`gui.view_roll_step_deg`, default 45). See [usage.md#view-roll](https://github.com/trailcode/EzyCad/blob/main/usage.md#view-roll) and `src/gui_mode.cpp` (`GUI::on_key`).

Related code: `src/gui_settings.cpp` (3D view navigation section). The experimental **Type** button + popup was removed; **Ctrl+click** on `SliderScalar` remains the supported way to type exact values.

### Suggestion

- Add a short note to **`ezycad_code_style.md`** or **`agents/README.md`** (or a tiny **`agents/imgui-notes.md`**) so future UI in tables does not repeat the mistake.
- Optional: extract a tiny helper, e.g. `defer_open_popup_after_table(bool& pending)` — only if we add more table-adjacent popups.

### Acceptance criteria

- [ ] Project docs mention ImGui popup + table ID stack **or** link to imgui#331 / upstream FAQ.
- [ ] No functional change required if docs-only.

---

## Metadata (do not paste)

- Created for agent/local drafting under `agents/issues/`.
- Repo: `trailcode/EzyCad`.
