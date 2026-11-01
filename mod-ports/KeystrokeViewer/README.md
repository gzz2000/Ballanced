# KeystrokeViewer for BallanceWeb

Source port of [Xenapte/MyBMLMods](https://github.com/Xenapte/MyBMLMods/tree/3718197500d2bc3e88cf20e5c27b377a4be6f22a/KeystrokeViewer) by BallanceBug, MIT license.
See provenance.json for exact upstream commit and the downloaded native reference.

Build after activating Emscripten 6.0.13:

```sh
emcmake cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBALLANCE_WEB_SDK=/absolute/path/to/Ballanced/sdk/wasm
cmake --build build-wasm
```

Import `KeystrokeViewer.bwmod` in Settings → User data → Plugins and restart.
The original config categories/properties are preserved. The source uses the
published BML interfaces and BGui::Text; it does not patch engine addresses.
The port maps BML headers/factories, uses 96 CSS reference DPI instead of Win32
GetDpiForSystem, and explicitly releases its BGui object during OnUnload.
A read-only `BML_WebKeystrokeViewerText` export exposes the actual sprite string for
acceptance tests; it does not mutate gameplay or bypass the rendering path.

The browser retains UTF-8 arrow labels instead of converting them through a Windows ANSI code page.


Compatibility: `ballanceweb-cxx-wasm32-v1`, minimum revision 1 (host `roundf`
service), Emscripten 6.0.13. Verified with BallanceWeb 20261101.14, build
`bfd7e2bd2dfe0ba3c2af5d44`. The host implements the existing `BGui::Text`
bitmap font API, including explicit newlines; all HUD text was inspected in
actual WebGL2 screenshots, including held arrows and after snapshot restore.

The shared test `../PositionViewer/qualify-hud.mjs` imports all three HUD ports
in a disposable browser profile, starts the original Level 10 as a profile-local
custom map, checks position and held/released input, collects a score item,
restores and recollects twice, verifies unload callbacks, then restarts with the
persisted all-keys config and checks a newly tracked key. Finally it removes
the three plugins through the UI and restarts to confirm they are no longer loaded. Build all three first.
Run from the workspace with `BALLANCE_TEST_URL=http://localhost:PORT node
research/doyagu/Ballanced/mod-ports/PositionViewer/qualify-hud.mjs` (one line).
Set `BALLANCE_HUD_BUILD_ROOT` and `BALLANCE_TEST_BROWSER` for other layouts.
The script records screenshots for visual inspection; successful text probes
alone do not establish that fonts render. It does not cover all maps or TAS tapes.

The text-change optimization compares the CK sprite itself, rather than a private
last-string cache, so restoring a snapshot or recreating the sprite cannot leave
a currently held key invisible.

Test scope: ordinary fast-mode Level 10 first score pickup, repeated restore
and recollection, held-key restore, config persistence and lifecycle. Life-item
collection, every map, full TAS trajectories and offline reopening were not
independently qualified by this port suite.
