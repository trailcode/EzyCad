# 3D viewer (Open CASCADE)

EzyCad's 3D viewport uses **Open CASCADE Technology (OCCT)** for displaying solids, sketches, the camera, lighting, and picking. For **mouse and keyboard** behavior, start with **[usage.md](usage.md)** ([View Controls](usage.md#view-controls)).

## Navigation

- **Orbit, pan, zoom** — See [Mouse Controls](usage.md#mouse-controls) in the usage guide.
- **View orbit / roll** — **NumPad 8**/**2**/**4**/**6** orbit (same axes as LMB drag); **Shift+NumPad 4** and **Shift+NumPad 6** roll around the screen axis. Step size is **Settings → 3D view navigation** (**View rotation step**). See [View navigation](usage.md#view-navigation).
- **Standard views** — **NumPad 5** snaps to the nearest main orthographic direction (top, bottom, front, back, left, right) and resets tilt relative to the model axes. See [View navigation](usage.md#view-navigation).

## On-screen aids

The **coordinate axes** (small triedron) and optional **view cube** are part of the OCCT-based viewer and help you stay oriented.

## Settings file

Background gradient, grid lines, and related **3D appearance** are stored under **`occt_view`** in your settings JSON. **View rotation step** is stored under **`gui`** (`view_roll_step_deg`) because it is edited in **Settings → 3D view navigation**. Full lists of keys: **[usage-settings.md — Settings file reference](usage-settings.md#settings-file-reference)**.

If you use scripts, **`ezy.occt_view_settings_json()`** returns a small JSON snapshot of some of these values — see **[scripting.md](scripting.md)**.

---

[Back to usage guide](usage.md) | [Settings](usage-settings.md)
