# PS3ME versions, tags and source map

This guide maps every tag currently published in this repository to the code snapshot and explains where the main parts of that code live.

## How to use a tag

A tag is a fixed snapshot of a commit. Checking out a tag shows the files exactly as they were at that point. The source tree is already grouped by subsystem in the complete code snapshots; preserve that layout when browsing older tags so build scripts and relative paths continue to match the release.

Use `git switch --detach <tag>` to inspect one version. Return to current development with `git switch main`. Read that snapshot's `README.md` and `RELEASE_NOTES_*.md` before building or comparing it.

## Published tags

| Tag | Commit | What it represents | Testing notes |
| --- | --- | --- | --- |
| `Alpha` | `2f2e9d0` | Early 01.14 source-import/bootstrap snapshot. It adds the source importer workflow and 01.14 release notes; treat it as an import milestone, not a numbered release. | Its README says 01.14 was not verified on PS3 hardware. |
| `V01.14` | `a9aefb9` | 01.14 MP3 stack repair: move decoding off SDL's small audio callback stack and use bounded prepared PCM. | Host and compiler checks are documented; that build was not independently verified on-console. |
| `V01.16` | `70a75ec` | Nokia UI/audio API packaging and compatibility restoration, graphics alignment fix, and Bounce Tales improvements. | Release notes and the 01.16 compatibility audit describe host, RPCS3 and reported hardware results; this is not a universal compatibility claim. |
| `V0.23` | `0c2d742` | The code and release notes identify this as **01.23**, including the XMB display-heartbeat and flip-synchronization work. The tag name is a typo/ambiguous version label. | Its release notes say physical PS3 verification was pending and RPCS3 was not run for that build. |

The current `main` branch contains later development than the numbered tags; its recent commits include 01.25 networking work. A branch head is not a released or tested version unless a release/tag and evidence say so.

### Correcting the 01.23 tag name

The canonical spelling should be `V01.23`, matching the existing `V01.14` and `V01.16` tags. The corrected name must point to the same commit as `V0.23`. Keep the old tag until any GitHub Release association and external links have been checked; deleting it can break those links.

After confirming `V01.23` is not already present, a maintainer can add the corrected name without rewriting code:

```bash
git fetch origin --tags
git show-ref --tags | grep -E 'refs/tags/V(0[.]23|01[.]23)
git tag V01.23 V0.23
git push origin refs/tags/V01.23
git rev-parse 'V0.23^{}'
git rev-parse 'V01.23^{}'
```

The two final commit IDs must match. Only remove `V0.23` after confirming no release or external link depends on it.

## Source code map

These paths are used in the full source snapshots. The exact files can differ by version; a subsystem may not exist yet in older tags.

| Path | Purpose |
| --- | --- |
| `ps3/src/` | Native PS3 application: browser/menu, controls, rendering, audio, storage, runtime integration and PS3 platform backend. |
| `ps3/include/` | Interfaces and shared declarations for the native PS3 application. |
| `ps3/java/` | Java classes and compatibility APIs packaged into the Java ME runtime. |
| `ps3/Makefile`, `ps3/javacall.mk`, `ps3/sfo.xml` | PS3 build configuration and application metadata. |
| `midp/` | phoneME MIDP runtime and PS3-specific build integration. |
| `pcsl/` | phoneME portability and system abstraction layer. |
| `vm/` | Java virtual machine and PS3/Cell adaptations. |
| `upstream/`, `vendor/`, `third_party/` | Imported upstream sources and dependencies; retain their notices and version pins. |
| `tools/` | Dependency preparation, patching, building, packaging and validation scripts. |
| `tests/`, `ps3/tests/` | Host-side regressions, compatibility checks and developer replay tests. |
| `assets/`, `patches/` | Project assets and maintained source patches. |
| `docs/`, `RELEASE_NOTES_*.md` | Guides, provenance, test evidence, known issues and version-specific changes. |

## What is safe to reorganize

Make new structural refactors on `main`, update build scripts and tests with the move, and publish a new tag only after validating the resulting build. Keep existing tags fixed so users can still reproduce the exact historical snapshots. For a clearer version list, update this guide and each release's notes rather than changing old tagged trees.

## Release checklist

For each future release, publish a tag pointing to the tested commit, release notes with the actual test environment and known issues, and the PKG/source artifacts with their provenance. State clearly whether results came from host tests, RPCS3 or a physical PS3.

git tag V01.23 V0.23
git push origin refs/tags/V01.23
git rev-parse 'V0.23^{}'
git rev-parse 'V01.23^{}'
```

The two final commit IDs must match. Only remove `V0.23` after confirming no release or external link depends on it.

## Source code map

These paths are used in the full source snapshots. The exact files can differ by version; a subsystem may not exist yet in older tags.

| Path | Purpose |
| --- | --- |
| `ps3/src/` | Native PS3 application: browser/menu, controls, rendering, audio, storage, runtime integration and PS3 platform backend. |
| `ps3/include/` | Interfaces and shared declarations for the native PS3 application. |
| `ps3/java/` | Java classes and compatibility APIs packaged into the Java ME runtime. |
| `ps3/Makefile`, `ps3/javacall.mk`, `ps3/sfo.xml` | PS3 build configuration and application metadata. |
| `midp/` | phoneME MIDP runtime and PS3-specific build integration. |
| `pcsl/` | phoneME portability and system abstraction layer. |
| `vm/` | Java virtual machine and PS3/Cell adaptations. |
| `upstream/`, `vendor/`, `third_party/` | Imported upstream sources and dependencies; retain their notices and version pins. |
| `tools/` | Dependency preparation, patching, building, packaging and validation scripts. |
| `tests/`, `ps3/tests/` | Host-side regressions, compatibility checks and developer replay tests. |
| `assets/`, `patches/` | Project assets and maintained source patches. |
| `docs/`, `RELEASE_NOTES_*.md` | Guides, provenance, test evidence, known issues and version-specific changes. |

## What is safe to reorganize

Make new structural refactors on `main`, update build scripts and tests with the move, and publish a new tag only after validating the resulting build. Keep existing tags fixed so users can still reproduce the exact historical snapshots. For a clearer version list, update this guide and each release's notes rather than changing old tagged trees.

## Release checklist

For each future release, publish a tag pointing to the tested commit, release notes with the actual test environment and known issues, and the PKG/source artifacts with their provenance. State clearly whether results came from host tests, RPCS3 or a physical PS3.
