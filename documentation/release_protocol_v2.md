# Release protocol v2

`reflex_master` is the source of truth for the SDK. The licensed `reflex` and
public `reflex_public` repositories are trimmed exports. Make release edits in
`reflex_master` and carry them forward through the exports.

## 1. Prepare the release in `reflex_master`

- Use a branch named `release-v<version>-prep`, for example
  `release-v0.9.0-prep`. An existing preparation branch such as `edge` is
  acceptable for a release already in progress.
- Prepare `changelog/<tag>.md` (for example, `changelog/v0.9.0.md`) and update
  `BREAKING_CHANGES.md` where migration is needed.
- Set root `version.txt` to the release tag on one line, for example `v0.9.0`.
  Commit the version, notes, and release changes on the preparation branch and
  push it. Set the next version only when preparing that release.

## 2. Run Test on the preparation revision

In `reflex_master`, run the **Test** workflow against the preparation branch.
All six platforms are selected by default. Enable **Tests: Templates**, **Tests: Examples**, and
**Tests: Run desktop apps** for the consumer and launch checks where supported.
If you also want the CMake matrix, run it separately; it does not cover Android
or Web. Wait for the checks to pass and record the tested `reflex_master` commit
SHA. If the release revision changes, run Test again on the new revision.

## 3. Export locally to licensed `reflex`

Start with a clean licensed checkout on the branch you intend to release from.
The exporter mirrors the source working tree and removes files that are absent
from the export. Run it from a clean `reflex_master` checkout at the tested SHA:

```shell
python scripts/export_sdk.py --dest D:/devt/public/reflex --identity reflexplusplus/reflex --dry-run
python scripts/export_sdk.py --dest D:/devt/public/reflex --identity reflexplusplus/reflex
```

In the licensed checkout, review the diff and confirm that root `version.txt`,
the matching changelog, and any breaking changes came across. Commit the
export, include the tested `reflex_master` SHA in the commit message, and push
the licensed branch manually. Keep release edits in `reflex_master`; if you
need a correction, make it there and export the corrected revision.

## 4. Build and publish from licensed `reflex`

In `reflexplusplus/reflex`, run **Release Public SDK** on the branch containing
the licensed export commit. Confirm that the workflow is running on that commit.
Choose the runners; the version comes from the selected revision's `version.txt`.
The workflow validates the version and changelog before building. It then builds
the libraries and CLI, exports the trimmed public source on CI, packages the
binaries, verifies the candidate, and publishes the verified source and assets to
`reflex_public`. CI creates the downstream tags. There is no manual public export
or tag push in this protocol.

The packaged libraries carry a per-platform copy of the release version for
`reflex version`. See [Public SDK releases](public_release.md) for runner setup,
packaging, and publication details.
