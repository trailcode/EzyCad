# 3D viewer (Open CASCADE)

EzyCad's 3D viewport uses **Open CASCADE Technology (OCCT)** for displaying solids, sketches, the camera, lighting, and picking. For **mouse and keyboard** behavior, start with **[usage.md](usage.md)** ([View Controls](usage.md#view-controls)).

## Navigation

- **Orbit, pan, zoom** — See [Mouse Controls](usage.md#mouse-controls) in the usage guide.
- **Num Lock** — **Num Lock off** is recommended for all **NumPad** view shortcuts (orbit, roll, zoom, axis snap). With **Num Lock on**, the OS may remap the keypad so those keys no longer match the docs; use main-row alternatives listed in [View navigation](usage.md#view-navigation).
- **View orbit / roll** — **NumPad 8**/**2**/**4**/**6** orbit (same axes as LMB drag); **Shift**+**NumPad 4**/**6**, **Shift**+main **4**/**6**, or **Shift**+**Left**/**Right** roll around the screen axis (repeat while held). Step size is **Settings → 3D view navigation** (**View rotation step**). See [View navigation](usage.md#view-navigation).
- **Zoom** — mouse wheel, **right-drag**, **NumPad +/-**, **Shift+=**, and main **-** share one path. Strength is **Zoom scroll scale** in Settings (`gui.view_zoom_scroll_scale`, default **4**). Hold **Shift** while zooming for a Blender-style finer step (**x0.1**). See [View Controls](usage.md#view-controls).
- **Standard views** — **NumPad 5** snaps to the nearest main orthographic direction (top, bottom, front, back, left, right) and resets tilt relative to the model axes. See [View navigation](usage.md#view-navigation).

## On-screen aids

The **coordinate axes** (small triedron) and optional **view cube** are part of the OCCT-based viewer and help you stay oriented.

## Settings file

Background gradient, grid lines, and related **3D appearance** are stored under **`occt_view`** in your settings JSON. **View rotation step** and **zoom scroll scale** are stored under **`gui`** (`view_roll_step_deg`, `view_zoom_scroll_scale`) because they are edited in **Settings → 3D view navigation**. Full lists of keys: **[usage-settings.md — Settings file reference](usage-settings.md#settings-file-reference)**.

If you use scripts, **`ezy.occt_view_settings_json()`** returns a small JSON snapshot of some of these values — see **[scripting.md](scripting.md)**.

---

[Back to usage guide](usage.md) | [Settings](usage-settings.md)
