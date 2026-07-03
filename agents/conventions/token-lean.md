# Keeping agent context lean

Goal: give assistants **only what they need** for the task at hand. Full style guides stay in `docs/`; load them when editing those files.

## Default load (most tasks)

1. Root [AGENTS.md](../../AGENTS.md) — pointers only (~20 lines).
2. [ascii-source.md](ascii-source.md) — when touching `src/` or `tests/`.
3. [docs/ezycad_code_style.md](../../docs/ezycad_code_style.md) — when writing C++ (do not duplicate in chat).

**Do not** auto-load: `workflows/release.md`, `outreach/`, `drafts/archive/`, or full `local-dev.md` unless building/releasing.

## Load on demand

| Task | Read |
| --- | --- |
| Build / test | [workflows/local-dev.md](../workflows/local-dev.md) — or root [README.md](../../README.md#building-instructions) |
| User-visible UI/settings | [user-docs-sync.md](user-docs-sync.md) + target `docs/usage-*.md` only |
| Docs build | [workflows/docs-build.md](../workflows/docs-build.md) |
| Release | [workflows/release.md](../workflows/release.md) |
| Specific issue/PR | One file under `drafts/issues/active/` or `drafts/prs/active/` |
| Forum posts | [outreach/discoverability.md](../outreach/discoverability.md) |

## Draft hygiene (saves tokens long-term)

- **Archive** merged drafts; keep `active/` small (open work only).
- Use **frontmatter** + short bodies in archive files (see [github-drafts.md](github-drafts.md)).
- Prefer **GitHub issue/PR links** over pasting long draft bodies into rules.
- Drop scratch files (`*-pr-body.txt`, duplicate templates).

## Cursor / IDE rules

Keep personal rules minimal: point at `AGENTS.md` and `agents/conventions/ascii-source.md`. Avoid copying whole `local-dev.md` or `release.md` into global rules.

## What stays out of the repo

IDE configs (`.cursor/rules/`, etc.) — use repo `agents/` as the portable source of truth.
