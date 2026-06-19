# Reflex Export And Distribution Spec

## Purpose

This spec separates two related but different concerns:

1. exporting `nsa/reflex` into public-facing mirror repositories
2. building and distributing binary libraries and tool packages

These should not be treated as one pipeline.

## Summary

### Task 1: Repo export

Source:

- `nsa/reflex`

Destinations:

- `reflexplusplus/reflex`
- `reflexplusplus/reflex_public`

Purpose:

- remove junk and internal-only material
- avoid exposing full private branch/history structure
- remove legally questionable or unwanted files such as `vst2api.h`
- publish clean snapshot repos intended for clients/public users

Trigger:

- manual only

### Task 2: Binary/tool distribution

Source:

- `nsa/reflex`

Outputs:

- versioned platform zip files for binaries
- versioned binary dependencies used by install flows

Purpose:

- package prebuilt libraries and selected tools
- support install/download flows for `reflex_public`
- provide the documentation binary as a separate dependency where needed

Trigger:

- can move to cloud later, but is a separate task from repo export

## Task 1: Repo Export

### Goal

Create clean public-facing mirror repositories from the current checked-out contents of `nsa/reflex`, without exposing the full original history and without carrying internal-only clutter.

### Important constraints

1. This must be manually triggered.
2. A human should decide when public repos are updated.
3. Export rules should be driven by `export.cfg`, migrated to `export.json`.
4. The old platform switch logic is not needed for repo export.
5. Platform-dependent export behavior should be removed.
6. It should instead support target-repo logic.

### Why manual trigger is required

The public repos are curated distributions, not automatic mirrors.

Reasons:

1. legal/sanitisation review may be needed
2. private/internal changes may be in flight
3. source releases should be deliberate
4. publishing to client/public repos is a product decision, not just a CI event

### Export targets

#### Target `reflex`

Destination:

- `reflexplusplus/reflex`

Intent:

- cleaned, sanitised source repo for clients with source access
- includes source needed by those clients
- removes junk, internal tooling, and disallowed materials

#### Target `reflex_public`

Destination:

- `reflexplusplus/reflex_public`

Intent:

- derived from the same source repo
- additionally strips selected source and internal code
- public/source-limited distribution

### Target-specific rule model

The old exporter supported `windows` / `macos` sections in `export.cfg`.

That should be dropped for repo export.

Replacement:

- target-specific export logic

Example:

- if target is `reflex_public`, exclude `src/reflex`
- if target is `reflex`, do not exclude `src/reflex`

This means the new JSON format should support target blocks, not platform blocks.

### Current exporter behavior that should be preserved

The current exporter lives in:

- [main.c](D:/devt/reflex/build/tools/ReflexExporter/resources/main.c)
- [engine.c](D:/devt/reflex/build/tools/ReflexExporter/resources/engine.c)

Useful behavior to preserve:

1. recursive tree walk
2. local config files applied per folder
3. inherited export rules
4. simple exclude/include semantics
5. deterministic output tree generation

### Current config features worth preserving

Existing `export.cfg` semantics:

- `exclude_folders`
- `exclude_types`
- `exclude_files`
- `include_types`

Those are sufficient for phase 1 of repo export.

### Features to drop from repo export

1. platform-specific config sections such as `windows` and `macos`
2. zip creation logic
3. rebuild logic
4. binary packaging concerns

### Proposed repo export config format

Replace `export.cfg` with `export.json`.

Recommended structure:

```json
{
  "exclude_folders": ["[old]", ".git", ".github", "output"],
  "exclude_types": ["zip", "cache"],
  "exclude_files": ["export.json", "installed.txt"],
  "include_types": ["gitignore"],
  "targets": {
    "reflex": {},
    "reflex_public": {
      "exclude_folders": ["reflex"],
      "exclude_files": ["vst2api.h"]
    }
  }
}
```

Note:

- the example above is illustrative only
- path decisions should be expressed at the correct folder level

### Rule precedence

For repo export, use:

1. inherited parent rules
2. local base rules
3. local target rules

Later layers override earlier layers.

### Compatibility recommendation

For migration, keep the existing replacement semantics for `exclude_*` lists rather than changing behavior.

That means:

1. child `exclude_folders` replaces inherited `exclude_folders`
2. child `exclude_types` replaces inherited `exclude_types`
3. child `exclude_files` replaces inherited `exclude_files`
4. `include_types` removes items from the effective excluded type set

### Destination repo publishing model

The destination repos should not mirror the full source history.

Recommended model:

1. generate a clean staging tree from the current source checkout
2. check out destination repo
3. replace destination contents, preserving only destination `.git`
4. commit and push a normal export commit

Recommended commit message:

```text
Export from nsa/reflex @ <source_sha>
```

### History policy

Keep export history in the destination repos, but do not attempt to replicate source repo history/branches/tags.

That gives:

1. visible change history for clients/public users
2. no exposure of original branch topology
3. simpler maintenance than history rewriting

### Proposed repo export tooling

Implement a Python CLI, for example:

- `tooling/export_repo.py`

Suggested commands:

```bash
python tooling/export_repo.py validate-config
python tooling/export_repo.py build --target reflex --output staging/reflex
python tooling/export_repo.py build --target reflex_public --output staging/reflex_public
python tooling/export_repo.py publish --target reflex --output staging/reflex --repo reflexplusplus/reflex
python tooling/export_repo.py publish --target reflex_public --output staging/reflex_public --repo reflexplusplus/reflex_public
```

### Proposed repo export workflow

Suggested GitHub Action characteristics:

1. manual `workflow_dispatch` only
2. build both target staging trees
3. optionally upload staging trees/manifests as artifacts for review
4. publish only when the operator confirms the run

Suggested jobs:

1. validate export config
2. build `reflex` export tree
3. build `reflex_public` export tree
4. publish `reflex`
5. publish `reflex_public`

## Task 2: Binary Libs And Tools Distribution

### Goal

Build and package binary libraries and selected tools into versioned platform archives.

This is related to export, but it is not the same thing as repo export.

### Current implementation

This is currently covered outside the old ReflexExporter config flow, mainly by:

- [export_bin.bat](D:/devt/reflex/export_bin.bat)
- [export_bin.command](D:/devt/reflex/export_bin.command)
- [build-release.yml](D:/devt/reflex/.github/workflows/build-release.yml)

This already shows that binary packaging is effectively a separate concern.

### Versioning requirement

Binary outputs need to be versioned from the repo version.

Current duplication:

- Git tag
- [version.txt](D:/devt/reflex/version.txt)

Recommendation:

- define one source of truth
- all packaging/versioned output should derive from that source

Preferred long-term direction:

1. tag and `version.txt` must be validated to match
2. workflow should fail if they diverge

### Documentation tool complication

Moving binary/tool building fully to cloud is complicated because the ReflexDocumentation app currently depends on access to `nsa/reflex_libraries`.

That means ReflexDocumentation should be treated as a separate dependency/stage, not folded blindly into the main repo export logic.

### Recommended binary/task split

Binary distribution should be broken into separate concerns:

1. core Reflex binary libraries
2. core buildable tools
3. ReflexDocumentation binary

### Important product direction

Ideal state for `reflexplusplus/reflex` clients:

- they build `bin/tools` themselves

But:

- ReflexDocumentation is not expected to be buildable by clients for the foreseeable future

So the documentation app should be distributed as a separate binary dependency.

## Recommended consumer model

### `reflexplusplus/reflex_public`

Recommended install/dependency model:

1. source repo contains public source only
2. install step downloads Reflex binary libs/tools
3. install step also downloads ReflexDocumentation binary as a separate dependency

In shorthand:

- `reflex_public` depends on Reflex bin libs/tools plus ReflexDocumentation binary

### `reflexplusplus/reflex`

Recommended install/dependency model:

1. source repo contains client source
2. install step builds tools locally where possible
3. install step downloads ReflexDocumentation binary separately

In shorthand:

- `reflex` depends on ReflexDocumentation binary
- local install/build flow should build other tools where feasible

## Consequences For The Spec

### Repo export spec should cover

1. `export.cfg` to `export.json` migration
2. target-specific filtering for `reflex` vs `reflex_public`
3. manual GitHub Action publishing to mirror repos
4. sanitised snapshot repo generation

### Repo export spec should not cover

1. platform zip packaging
2. bin/tools binary archive layout
3. documentation binary build details
4. release artifact publishing

Those belong to the binary/tool distribution task.

## Recommended next design artifacts

### For Task 1

Create:

1. `tooling/export_repo.py`
2. `tooling/export_targets.json`
3. JSON schema for `export.json`
4. manual GitHub Action workflow for repo export

### For Task 2

Create separately:

1. binary packaging/versioning spec
2. documentation binary dependency spec
3. install-flow updates for `reflex_public`
4. install-flow updates for `reflex`

## Open questions

1. Exactly which paths should be excluded only for `reflex_public`?
2. Exactly which internal tooling should be removed from both public-facing repos?
3. Should the manual export workflow publish both repos in one run or allow selecting one target at a time?
4. What should be the single source of truth for versioning: tag, `version.txt`, or generated release metadata?
5. How should the ReflexDocumentation binary be published and versioned relative to the core repo version?

## Recommended immediate next step

Implement Task 1 first:

1. migrate a small subset of `export.cfg` files to `export.json`
2. scaffold `tooling/export_repo.py`
3. support only target-aware repo export
4. ignore platform logic entirely
5. generate staging trees for `reflex` and `reflex_public`

Then write a separate spec for Task 2 binary/tool distribution.
