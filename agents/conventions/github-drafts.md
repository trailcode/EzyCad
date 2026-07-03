# GitHub issue and PR drafts

Repo-local markdown drafts for [trailcode/EzyCad](https://github.com/trailcode/EzyCad). These are **not** a substitute for GitHub tracking; they keep wording, scope, and test plans in-repo for agents and contributors.

## Layout

```
agents/drafts/
  issues/
    README.md
    _template.md
    active/     # open or not yet filed on GitHub
    archive/    # merged, closed, or resolved
  prs/
    README.md
    _template.md
    active/
    archive/
```

## Naming (GitHub-first)

| Situation | Filename pattern | Example |
| --- | --- | --- |
| Issue filed on GitHub | `gh-NNN-short-slug.md` | `gh-134-linear-edge-automatic-splitting.md` |
| PR filed on GitHub | `gh-NNN-short-slug.md` (use **PR** number in `prs/`) | `gh-145-sketch-delta-undo-redo.md` |
| Not yet filed | `draft-short-slug.md` | `draft-src-prefix-refactor-sketch-recorder.md` |

Use lowercase kebab-case slugs. Keep slugs short but recognizable.

## Frontmatter (recommended)

Add YAML at the top of each draft when you know the numbers:

```yaml
---
github_issue: 144
github_pr: 145
status: active
paired_draft: ../prs/active/gh-145-sketch-delta-undo-redo.md
---
```

`status`: `active`, `merged`, `closed`, or `abandoned`.

## Workflow

1. Copy `_template.md` into `active/` with the correct name.
2. Fill in title, body, acceptance criteria, and test plan.
3. After opening on GitHub, rename `draft-*.md` to `gh-NNN-*.md` and add frontmatter.
4. Pair issue and PR drafts; cross-link in **Related** sections.
5. When merged or no longer needed, move to `archive/`.

## Templates

- Issue: [drafts/issues/_template.md](../drafts/issues/_template.md)
- PR: [drafts/prs/_template.md](../drafts/prs/_template.md)

## Related

- User-visible changes: [user-docs-sync.md](user-docs-sync.md)
- Local dev and docs build: [workflows/local-dev.md](../workflows/local-dev.md), [workflows/docs-build.md](../workflows/docs-build.md)
