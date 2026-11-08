# Version and source organization guide

This document explains how to navigate PS3ME's historical releases without rewriting the code snapshots stored in Git tags.

## What a tag means

A Git tag points to a particular commit (or to an annotated tag object). It is **not** a separate working directory or a mutable branch. Files displayed under a tag are the exact contents of that historical snapshot. Reorganizing those files would require new commits and would no longer be the original release snapshot.

For this reason, keep historical tags unchanged and organize documentation on `main`. Use release notes to explain what each snapshot introduced.

## Documented release milestones

| Version | Purpose / documented changes | Evidence |
| --- | --- | --- |
| 01.05–01.07 | Earlier PS3-specific guides; historical behavior may differ from current releases | `docs/archive/README_PS3_01.05.md`, `README_PS3_01.06.md`, `README_PS3_01.07.md` |
| 01.16 | Nokia API packaging, Nokia Sound, MMAPI tone support, graphics compatibility fixes, and Bounce Tales validation | `RELEASE_NOTES_01.16.md`, `docs/COMPATIBILITY_AUDIT_01.16.md` |
| 01.17–01.19 | Game browser UI changes, English/Brazilian Portuguese selection, loading screen, XMB icon updates and redraw optimizations; real PS3 menu freeze and PS-button crash reported in those builds | `RELEASE_NOTES_01.19.md` |
| 01.23 | Current version identified by the main README; earlier v1.19 regressions were subsequently reported fixed | `README.md` |

**Caution:** The table documents milestones from available project notes; it is not a verified inventory of all existing Git tags. Avoid inferring missing version features from tag names alone.

## Source layout: how to navigate

- `README.md` — current installation instructions, controls, save paths, credits and warnings.
- `docs/README.md` — documentation entry point and historical guides.
- `docs/archive/` — archived guides tied to older versions; do not use as instructions for current builds.
- `RELEASE_NOTES_*.md` — version-specific changes, limitations and test results.
- `docs/COMPATIBILITY_AUDIT_01.16.md` — 01.16 compatibility testing and supporting evidence.
- `docs/THIRD_PARTY_NOTICES.md` — third-party attribution and licensing.
- `docs/AI_DISCLOSURE.md` — AI-assisted development disclosure.
- `.gitignore` — generated build products, local caches and other untracked files.

## Recommended future layout

For new code on `main`, gradually group PS3 platform glue, Java ME runtime integration, UI, input, audio, build tooling, tests, and documentation into clearly named directories **only after identifying actual dependencies and verifying builds**. Do not move files speculatively: PS3 toolchains and scripts may depend on relative paths.

For each future release, publish:
1. A tag pointing to the tested commit.
2. Release notes describing changes, test environment, and known issues.
3. A PKG artifact if available, with its provenance.
4. Any compatibility regressions and whether they were reproduced on physical hardware.

## Renaming a tag

To rename `v0.23` to `v01.23` **without changing its target commit**, first inspect both names and confirm `v01.23` does not already exist:

```bash
git fetch origin --tags
git show-ref --tags | grep -E 'refs/tags/v(0\.23|01\.23)$'
```

If `v01.23` does not exist, create it from the old tag and push it:

```bash
git tag v01.23 v0.23
git push origin refs/tags/v01.23
```

Verify the peeled commit IDs match:

```bash
git rev-parse 'v0.23^{}'
git rev-parse 'v01.23^{}'
```

Only after verifying the new tag and checking whether GitHub Releases or external links reference the old name, delete the old remote tag:

```bash
git push origin :refs/tags/v0.23
git tag -d v0.23
```

If `v0.23` is annotated, `git tag v01.23 v0.23` creates a new lightweight tag to the original tag's peeled target; use an annotated replacement if preserving tag-message semantics matters. GitHub Releases may need their tag association updated separately. Do not delete a release asset or rewrite an existing `v01.23` tag.
