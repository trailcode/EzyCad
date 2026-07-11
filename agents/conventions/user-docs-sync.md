# User-facing documentation sync

Use this when you add or change **user-visible** behavior (UI, settings, shortcuts, sketch/shape workflows, persisted JSON keys).

Style rules live in **[docs/ezycad_doc_style.md](../../docs/ezycad_doc_style.md)**. This file is the **what to update** checklist for agents and contributors.

## When to update docs

Update user docs in the **same PR / branch** as the code when you **add or change** anything that affects:

- **Settings pane** or **Options** panel controls
- **Persisted keys** in `ezycad_settings.json` (`gui.*`, `occt_view.*`)
- **Sketch / shape / view** workflows users see in the app
- **Keyboard shortcuts**, tooltips, or Help links
- **Scripting** surface (`ezy.occt_view_settings_json()`, etc.)

Skip user-guide updates for internal-only refactors (PIMPL, naming, line endings in `src/`, etc.) unless behavior or saved data changes.

## Files to check

| Change type                             | Primary docs                                                                               |
| --------------------------------------- | ------------------------------------------------------------------------------------------ |
| New/changed **Settings** or **Options** | `docs/usage-settings.md` (pane list, Options section, **`gui` / `occt_view` JSON tables**) |
| Sketch tools, snap, dimensions          | `docs/usage-sketch.md`; cross-links from `usage-settings.md`                               |
| 3D view, grid, navigation               | `docs/usage-occt-view.md`, `docs/usage.md` (view sections)                                 |
| General UI, lists, modes                | `docs/usage.md`                                                                            |
| Lua / Python / settings JSON API        | `docs/scripting.md`                                                                        |
| Build / OCCT / wasm                     | `docs/building-occt.md`                                                                    |
| Notable user-facing fix or feature      | `CHANGELOG.md` under `[Unreleased]`                                                        |

## Minimum checklist (settings / preferences)

When adding or renaming a **Settings -> Sketch** (or other pane) control:

- [ ] **Settings pane** bullet list matches on-screen labels and ranges
- [ ] **Options** section updated if the same control appears live in **View -> Options**
- [ ] **`gui` JSON reference table** in `usage-settings.md` lists every new/changed key (type, range, default)
- [ ] Cross-link from the feature guide (e.g. `usage-sketch.md#sketch-snapping`) if users need context
- [ ] `CHANGELOG.md` `[Unreleased]` entry when the change is user-visible
- [ ] Optional: `agents/drafts/issues/` or `agents/drafts/prs/` draft lists doc files in **Test plan** / **Files touched**

## Verify

See [workflows/docs-build.md](../workflows/docs-build.md) or CI [`.github/workflows/docs.yml`](../../.github/workflows/docs.yml).

## Related

- Example: [gh-111-sketch-snap-unification-and-docs.md](../drafts/issues/archive/gh-111-sketch-snap-unification-and-docs.md)