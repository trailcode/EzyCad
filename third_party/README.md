# Third-party dependencies

Vendored sources and binaries for EzyCad live under **`third_party/`** (lowercase, underscore). CMake and scripts assume this layout.

The **`thirdParty/`** directory name (camel case) is **not** used by this project; it may appear in older docs or external tool defaults. Ignore or remove stray `thirdParty` trees to avoid confusion with Conan/NuGet outputs (see repo `.gitignore`).

## ImGuiColorTextEdit

Required for the Lua and Python script consoles (`TextEditor` widget).

- Expected path: `third_party/ImGuiColorTextEdit/TextEditor.cpp` (and headers).
- Upstream: [BalazsJako/ImGuiColorTextEdit](https://github.com/BalazsJako/ImGuiColorTextEdit) (clone or extract the repository contents into `third_party/ImGuiColorTextEdit/`).

If this folder is missing, CMake fails with a short message pointing here.

If `third_party/ImGuiColorTextEdit` is absent, CMake fetches [BalazsJako/ImGuiColorTextEdit](https://github.com/BalazsJako/ImGuiColorTextEdit) (branch `master`) into the build tree on configure. Vendoring the folder in the source tree is still recommended for offline builds and reproducibility.
