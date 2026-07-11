---
github_issue: 187
status: active
---

# Shrink GitHub Pages wasm demo footprint

**Suggested labels:** `enhancement`, `wasm`, `docs`, `chore`

---

## Title (GitHub)

Shrink trailcode.github.io EzyCad wasm demo size

## Body (GitHub)

### Summary

The published EzyCad wasm demo under `trailcode.github.io/EzyCad/` should be as small as practical so the GitHub Pages site stays lean for visitors and hosting.

### Problem

- Demo assets (`.js` / `.wasm` / `.data` and related HTML) can grow large with OCCT + Emscripten builds.
- Repo helper scripts were consolidated under `scripts/` in EzyCad; publishing size is a separate concern on the Pages repo.
- `scripts/sync-github-pages-html.ps1` only syncs HTML today; binary payload size needs an explicit trim strategy.

### Proposed scope

- Audit current `trailcode.github.io/EzyCad/` payload sizes (HTML, JS, WASM, data).
- Prefer Release / size-optimized Emscripten flags already used for demos; document the publish recipe.
- Drop unused assets, old cache-busted copies, and redundant files from the Pages tree.
- Optionally compress or split large packs if it helps first load without breaking the demo.
- Keep `EZYCAD_WEB_CACHE` bump workflow when replacing binaries.

### Acceptance criteria

- [ ] Documented before/after size for the EzyCad Pages folder (or main downloadable assets).
- [ ] Demo still loads and runs on https://trailcode.github.io/EzyCad/ (or current URL).
- [ ] Publish steps updated in `agents/workflows/local-dev.md` / building docs if the recipe changes.
- [ ] No unrelated app feature changes in the size-trim PR.

### Related

- `scripts/sync-github-pages-html.ps1`
- EzyCad `web/EzyCad.html` cache bust (`EZYCAD_WEB_CACHE`)
- Unit-test / scripts consolidation: #186 / https://github.com/trailcode/EzyCad/pull/188
- Draft: `agents/drafts/issues/active/gh-187-shrink-github-pages-wasm.md`
