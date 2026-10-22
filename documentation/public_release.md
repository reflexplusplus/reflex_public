# Public SDK releases

Public releases are built from `reflexplusplus/reflex`. They do not check out
or require `reflex_master`. The legacy master workflows remain available.

## Run a release

Run **Release Public SDK** (`release_public_sdk.yml`) in the licensed
repository. Select the source branch or tag and enter a version such as
`v0.7.0`. A matching, completed `changelog/v0.7.0.md` must be committed.
There is no requirement to merge the selected revision to `main`.

The manual form has three runner checkboxes, all checked by default. Checked
uses self-hosted runners; unchecked uses GitHub-hosted runners:

| Checkbox | Target platforms | GitHub-hosted alternative |
| --- | --- | --- |
| Windows | Windows | `windows-latest` |
| macOS | macOS and iOS | `macos-15` |
| Linux | Linux, Android and Web | `ubuntu-latest` |

The consolidated **Test** workflow (`test.yml`) uses the same three controls.
**Check Runners** (`check_runners.yml`) has only `Check: Windows`, `Check: macOS`
and `Check: Linux` selection checkboxes and always checks self-hosted runners.
All use the shared
`.github/workflows/select-runners.yml` workflow to select runners. This is an
explicit choice: a selected self-hosted runner that is offline leaves the job
queued; uncheck its box to use GitHub-hosted instead. Small selection, preparation,
assembly and publication jobs remain GitHub-hosted.

Test replaces Build Check and the separate macOS Test Matrix. Its form groups
platform selection under `Build:`, optional checks under `Tests:`, and runner
selection under `Runner:`. `Tests: Templates`, `Tests: Examples` and
`Tests: Run desktop apps` are all off by default. With them off, Test still
bootstraps the host CLI and builds Debug/Release libraries for each selected
platform, without packaging or publication.

Shared workflow orchestration and summaries run directly in Python. Windows setup
uses built-in PowerShell; the new workflows do not require Bash on Windows.

Templates build in Debug on supported target platforms; the console template
is desktop-only. Examples builds the six selected desktop examples. Run desktop
apps applies only to selected templates/examples and currently runs on Windows
and macOS; checking it alone does not select any projects.

`Build: CMake` switches both the library and consumer builds to CMake. The CLI
bootstrap already uses its checked-in CMake project in either mode. CMake mode
generates the root library `CMakeLists.txt` from `project.cfg`, then builds Debug
and Release libraries. It builds examples using their checked-in CMakeLists and
created templates using their shipped, substituted CMakeLists, without regenerating
those consumer files. The root CMakeLists is overwritten in the checkout, including
when run locally; no automatic restoration is performed. CMake mode selects only
Windows, macOS, Linux and iOS from the platform checkboxes: Android and Web are
omitted because their library CMake generation is not supported. iOS builds both
device and simulator targets. With CMake unchecked, builds use the native tooling.
On macOS, CMake uses Ninja when it is available on PATH, otherwise Xcode; iOS
always uses Xcode. The selected Apple generator is reported in the test log.
Generated iOS CMake targets carry the architectures configured in `project.cfg`
for each build configuration. Shared CMake helpers default iOS targets to arm64
when no CMake architecture was supplied, for both devices and simulators.

Consumer checks reuse the initial library build. Optional plist checks run
with macOS templates, and test logs are retained as diagnostic artifacts.

Check Runners replaces the separate Windows and macOS runner-smoke workflows.
It reports runner and toolchain details, applies the shared build setup,
bootstraps the CLI through CMake, and checks the resulting executable. It does
not build libraries or example/template apps, or create package archives.

The workflow builds Debug and Release libraries for Windows, macOS, Linux,
iOS, Android and Web. It builds a fresh CLI on each host and signs/notarizes the
macOS CLI. AAX and VST2 are disabled through the shared builder's defaults.
Generation and build settings are passed through command-local `--variables`;
the workflows do not set or unset persistent CLI preferences.

Desktop tool archives contain only the CLI and a version marker. The CLI is
updated with the installed SDK, so public users do not need CMake or a local CLI
rebuild after installation. Project Creator and Resource Builder remain available
as source for optional local builds; they are not built or packaged by this workflow.

Source files are selected by the `reflex_public` section of `export_rules.json`.
Repository exports then filter the selected paths through Git's ignore rules
in one batch. Tracked files remain eligible; ignored untracked files are omitted,
including explicit exceptions. Git's local and global ignore settings also apply.
Binary packaging and mirroring the verified snapshot do not apply this filter.
The same section's `exclude_files` lists the exact repo-relative paths of VM,
AAX and VST2 archives and debug-symbol files to omit from binary packages.
Source is published as `reflex-source.zip`, containing the exact source tree
used during verification. Libraries and the host CLI are separate packages;
complete `reflex-dist-*` archives are not produced.
Current documentation archives are attached when available from the existing
documentation release. These pre-signed documentation apps are not rebuilt or
re-signed here.

Linux libraries and CLI tools use `.tar.gz`. Windows and macOS packages,
the common source snapshot and other target libraries remain ZIPs.
Documentation assets are forwarded unchanged, including Linux `.tar.gz`
or `.tar.xz` packages, and remain optional separate downloads.

Verification extracts the packaged SDK and uses its packaged host CLI to
generate/build the public C++ templates for each supported target platform.
Each verification job downloads only its required archives and the common
candidate manifest. All six platforms receive the verified public source,
their target libraries and the appropriate host CLI.
Each job checks the downloaded archives against the original manifest.
It does not rebuild the CLI or use the checkout's development CLI. Native desktop
and iOS jobs also run the existing CMake distribution-consumer checks. No
application is launched.

The `public-candidate` artifact contains the source snapshot, release assets,
notes, source revision and SHA-256 checksums. It is retained for 14 days. If any
build or verification job fails, publication does not run; artifacts remain
available for diagnosis.

Platform jobs upload archives only. Assembly requires all six library archives and
three host CLI archives, then writes one final `candidate.json` manifest.
Verification checks its file hashes, and publication selects release assets
from that same manifest. All jobs use the workflow's selected source revision.
Publication downloads the complete candidate and checks every file hash before
publishing.

`reflex install` and CMake consume these same modular packages. The website
uses `reflex-source.zip` as the release's core content and labels the library,
CLI and documentation assets by platform. Its tag-archive fallback remains
available for older releases and the legacy workflow. After deploying website
changes, pull an existing release again to refresh its cached asset mapping.

The CMake `reflex_fetch_dist` helper retains its entry point but fetches
`reflex-source.zip`; subsequent `find_package(Reflex)` downloads target libraries
and host tools through `FetchReflexLibs.cmake`. It no longer needs a complete
distribution archive. Source archives without a Git origin default to the public
repository; `REFLEX_GITHUB_REPO` can select another repository.

## Automatic publication

Once all platform verification jobs pass, the same workflow automatically
publishes the verified artifacts. There is no second workflow or manual approval
step. Publication checks the source revision and checksums; it does not rebuild
or regenerate the public export.

Publication creates the version tag in `reflex` at the verified source commit,
mirrors the public source snapshot into `reflex_public`, tags that exported
commit, and uploads the matching public release assets. After public GitHub
publication succeeds, it creates or updates a notes-only release in licensed
`reflex`, using the same changelog and prerelease status. Binary assets are
uploaded only to `reflex_public`. Existing tags are never
moved. Repeating publication is allowed when the existing public tag is still
at the public branch head and the candidate contents match.

After GitHub publication, the shared website notification requests a mirror of
the release and polls until it is available through reflexplusplus.dev, which
the CLI uses for installation and release discovery. If that confirmation
fails, the workflow fails while the GitHub release remains published.

## Repository setup

- Introduce `release_public_sdk.yml` on the licensed repository's default
  branch so its manual Run workflow button is available. Runs can then
  select `dev` or another branch.
- Enable Actions and make the selected self-hosted runners available to `reflex`,
  with the standard `self-hosted` and OS labels (`Windows`, `macOS`, `Linux`).
  CMake and the native platform toolchain must be installed. Shared setup reuses
  Python 3.10+ when available and provisions Python 3.10 otherwise. Windows checks
  `python`, `python3` and the `py -3` launcher; Unix checks `python3` and `python`.
  Tools must be visible to the runner process, which may have a different PATH
  from an interactive terminal.
  Linux setup checks the required Ubuntu/Debian packages and installs only missing
  packages. Installation requires root or noninteractive sudo; if neither is
  available, the job fails with the missing package list instead of prompting.
  Android setup reuses JDK 17 and existing SDK tools, and installs missing
  platform-tools, Android platform 37.0 (`platforms;android-37.0`), SDK Build Tools
  36.0.0, NDK 28.2.13676358 and SDK CMake 3.22.1 components.
  Web setup validates configured `LLVM_ROOT` and `WASI_SYSROOT` paths (the latter
  defaults to `LLVM_ROOT/share/wasi-sysroot`). Without configured paths, it downloads
  checksum-verified WASI SDK 34 once into
  `RUNNER_TOOL_CACHE/reflex-wasi/wasi-sdk-34.0-x86_64-linux` and reuses it on later
  jobs. This cache survives jobs on persistent self-hosted runners; fresh hosted
  runners provision their own copy. Concurrent cache provisioning is serialized.
- Make the existing macOS signing secrets available to `reflex`, either as
  repository secrets or organization secrets granting it access: `APPLE_CERT_P12`,
  `APPLE_CERT_PASSWORD`, `APPLE_TEAM_ID`, `APPLE_NOTARY_USER` and
  `APPLE_NOTARY_PASSWORD`.
- Set `EXPORT_PUSH_TOKEN` with Contents read/write access to
  `reflexplusplus/reflex_public`. The workflow's own token creates the source
  tag and notes-only release in `reflex` and needs Contents write permission there.

The licensed export includes scripts, tests, `export_rules.json` and `.github`,
excluding the legacy `build-release.yml` and `export-repos.yml` workflows.
The public export removes the release infrastructure and rule file. Check Runners
checks use local builds and do not require distribution-signing secrets.

Because the licensed export now includes workflow files, a token used to push
that checkout must also be permitted to update workflows. This also applies if
the legacy exporter is used to push the updated licensed export.

## Helper commands

`scripts/export_reflex_public_sdk.py` provides `check`, `package`, `assemble`,
`verify` and `publish` commands. Normally use the workflow, which sets up the
platform toolchains and enforces the successful-build requirement before
publication. Run a command with `--help` to see its inputs.

The release helper imports only `ci_build` and `export_common` from this repo.
Release notes must have the matching version headline, a nonempty body and no
unfilled title or summary placeholders. The legacy release-notes editorial
checks are not applied by this workflow.

On Linux, `reflex install` uses `tar` for compressed tar archives and Python 3
for ZIP archives, including the common source download. No `unzip` executable
is required. Extraction preserves Unix executable permissions and symlinks.
The website release API must recognize the Linux archive filenames; its
implementation is maintained separately.
