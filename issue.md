**Track on GitHub:** https://github.com/trailcode/EzyCad/issues/77

---

## Summary

Improve **reproducible native (Windows) configures** for **clean out-of-source build directories** and for **clones without vendored ImGuiColorTextEdit**, so contributors and CI do not depend on shell working directory or manual third-party copies.

## Background

- NuGet `install` previously used a **relative** `-o thirdParty`, so packages could land outside `${CMAKE_BINARY_DIR}` when CMake was run from varying working directories. That broke `file(COPY ...)` during configure for fresh build trees.
- **ImGuiColorTextEdit** was required only under `third_party/`; missing folder caused configure failure with no automatic recovery.

## Completed

- NuGet: `-OutputDirectory "${CMAKE_BINARY_DIR}/thirdParty"`.
- If `third_party/ImGuiColorTextEdit/TextEditor.cpp` is missing, **FetchContent** from [BalazsJako/ImGuiColorTextEdit](https://github.com/BalazsJako/ImGuiColorTextEdit) with staged copy under the build dir and include path adjusted so existing `#include "ImGuiColorTextEdit/TextEditor.h"` keeps working.
- **Pinned `GIT_TAG`** to **`ca2f9f1462e3b60e56351bc466acda448c5ea50d`** (upstream has no release tags; shallow clone).
- `third_party/README.md` describes vendoring vs fetch.
- **README.md**: end section **In-tree third-party libraries** documents `third_party/` (Dear ImGui at `third_party/imgui/`, ImGuiColorTextEdit, json, tinyfiledialogs) vs **OCCT** built outside the tree; notes the ImGuiColorTextEdit pin and NuGet `thirdParty` output dir.

## Remaining (optional)

- [ ] Add or extend **GitHub Actions** to run a **native Windows configure** (and optionally build) when OCCT + 3rd-party paths are available (self-hosted runner, cache, or documented secrets).
- [ ] Add **`.gitattributes`** for `*.md` line endings if the team wants consistent LF in docs.

## Acceptance criteria

- From an **empty** build directory: `cmake -S <src> -B <build> ...` completes without requiring NuGet output to already exist beside the source tree.
- Clone **without** `third_party/ImGuiColorTextEdit` still configures (network allowed for first-time FetchContent).
- Configures are **deterministic** for ImGuiColorTextEdit fetch (pinned commit, not a moving branch).

## Suggested branch name (historical)

`fix/cmake-reproducible-native-deps`
