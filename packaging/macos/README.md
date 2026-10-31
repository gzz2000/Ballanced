# Native macOS application bundle

This directory packages the native arm64 runtime as a self-contained macOS
application. It does not grant redistribution rights for the original
Ballance game data; build with assets from a legally obtained installation.

Build Ballanced's `macos-arm64-runtime` stage and a native BML+ checkout first.
By default, the script expects BML+ next to this repository:

```sh
./packaging/macos/build-app.sh
```

The following environment variables can override build inputs:

- `BALLANCED_STAGE`
- `BML_PLUS_ROOT`
- `BML_PLUS_LIBRARY`
- `BML_PLUS_DEFAULTS`
- `BML_PRACTICE_TOOLS`
- `BML_SPEEDRUN_SHORTCUTS`
- `BALLANCE_TAS_MOD`
- `BML_LEGACY_TAS_RUNTIME`

The bundle includes the matching native PracticeTools, SpeedrunShortcuts,
BallanceTAS and LegacyTASRuntime modules. The launcher updates only these managed
binaries, preserving user maps, scores, configuration and third-party modules.
For an isolated test installation, set `BALLANCE_USER_DATA_DIR` before launching.
The current build targets macOS 15 or newer on Apple Silicon.

Mutable state is stored under
`~/Library/Application Support/Ballance`; the application bundle remains
read-only. `Contents/Resources/Defaults/Database.tdb` is only the factory seed;
the game never opens it directly and always reads and writes the copy in
Application Support. Rendering follows the active display's synchronized presentation
rate while input and gameplay use the deterministic fixed scheduler.
