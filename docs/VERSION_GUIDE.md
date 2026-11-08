# PS3ME versions, tags and source map

This guide explains what each published PS3ME tag contains and where to find the main parts of its source.

## Inspecting a version

A Git tag is a fixed snapshot of one commit, not a folder that contains its own version directory. To inspect a release, switch to its tag and keep the source tree intact so the build scripts and relative paths continue to work:

```bash
git fetch origin --tags
git switch --detach V01.23
```

Return to current development with `git switch main`. Read that snapshot's `README.md` and `RELEASE_NOTES_*.md` before building it. Use the attached source ZIP on the matching GitHub Release when you need the exact release package source and build materials.

## Published tags

| Tag | Commit | Purpose | Validation notes |
| --- | --- | --- | --- |
| `Alpha` | `2f2e9d0` | Historical source-import workflow milestone for 01.14; not a numbered emulator release. | Do not treat this helper commit as the 01.14 release source. |
| `V01.14` | `a9aefb9` | Experimental 01.14 release snapshot; includes the MP3 stack repair using bounded prepared PCM. | Host/compiler checks are documented; that build was not independently verified on a physical PS3. |
| `V01.16` | `70a75ec` | Restored Nokia UI/audio API packaging, graphics alignment fix, and Bounce Tales compatibility improvements. | See its release notes and compatibility audit for the reported host, RPCS3, and hardware results; these do not guarantee every game works. |
| `V01.23` | `0c2d742` | XMB display heartbeat and flip-synchronization work, plus the 01.23 menu and UI improvements. | The release notes say physical PS3 verification was pending and RPCS3 was not run for this build. |

The 01.23 release is published as **PS3ME 01.23 — XMB display heartbeat**. The tag uses the same commit as the former `V0.23` spelling; the rename does not change code or release assets.

The current `main` branch contains newer development than these releases. A branch head is not a released or tested version unless it has a matching release tag and validation evidence.

## Source layout

The complete source snapshots group files by subsystem rather than duplicating the entire tree into a separate folder for every version. Older tags can lack files added later, so compare only paths that exist in both snapshots.

| Path | Purpose |
| --- | --- |
| `ps3/src/` | Native PS3 application: menus, controls, rendering, audio, storage, runtime integration, and platform backend. |
| `ps3/include/` | Shared native interfaces and declarations. |
| `ps3/java/` | Java classes and compatibility APIs included in the Java ME runtime. |
| `ps3/Makefile`, `ps3/javacall.mk`, `ps3/sfo.xml` | PS3 build rules and application metadata. |
| `midp/` | phoneME MIDP runtime and PS3-specific build integration. |
| `pcsl/` | phoneME portability and system abstraction layer. |
| `vm/` | Java virtual machine and PS3/Cell adaptations. |
| `upstream/`, `vendor/`, `third_party/` | Imported upstream sources and dependencies; retain their licenses, provenance, and version pins. |
| `tools/` | Dependency preparation, patching, building, packaging, and validation scripts. |
| `tests/`, `ps3/tests/` | Host regressions, compatibility checks, and developer replay tests. |
| `assets/`, `patches/` | Project assets and maintained source patches. |
| `docs/`, `RELEASE_NOTES_*.md` | Guides, provenance, test evidence, known issues, and version-specific changes. |

Keep historical tags unchanged so people can reproduce their original snapshots. Make structural code refactors on `main`, update build scripts/tests and this map with each move, then publish a new tag only after validating that snapshot. The release checklist is to identify the exact tag, summarize its purpose, and state whether evidence came from host tests, RPCS3, or a physical PS3.

