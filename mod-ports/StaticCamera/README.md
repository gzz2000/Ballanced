# StaticCamera for BallanceWeb

Source-level port of BallanceBug (Xenapte)'s StaticCamera 0.1.0, from
https://github.com/Xenapte/MyBMLMods/tree/3718197500d2bc3e88cf20e5c27b377a4be6f22a/StaticCamera .
Original MIT license is retained. See provenance.json for the downloaded PE identity.

The original fixed camera offset, Shift+Left/Right quarter turns, Right Shift
synchronization and BML settings remain. Changes are SDK spelling/pointer
adaptations, explicit VxVector assignments, WASM entry/destructor exports, null
checks, and detaching the camera on unload. The host CK context owns the camera.
No engine layout changes or direct object-memory access are required.

## Build

From the Ballanced root with matching Emscripten 6.0.13 and public SDK:

```sh
emcmake cmake -S mod-ports/StaticCamera -B build/mods/static -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBALLANCE_WEB_SDK="$PWD/sdk/wasm"
cmake --build build/mods/static
```

Import `build/mods/static/StaticCamera.bwmod` in Settings → User data → Mods,
then restart. Settings are in the in-game BML Mod Options → Static Camera → Main.
The built package declares ABI v1, minimum revision 1, and requires no other mod.

## Validation and limitation

Chrome/WebGL2 on the revision-1 host passed install/restart, Level 1 movement,
fixed follow offset, 90-degree rotation, synchronization with InGameCam, actual
BML enable/disable config callbacks, normal exit/OnUnload and removal/restart.
The same ordinary behavior also passed on revision 0 with the original build.

**PracticeTools snapshot compatibility is incomplete.** Saving, rotating the
camera, then restoring successfully restores the engine snapshot but does not
restore the plugin's private `cam_pos_diff_`; its next OnProcess overwrites the
restored camera position. This is a measured failure, not a fully passing
save/restore port. Right Shift resynchronizes the view but is not a snapshot fix.
A host-supported plugin snapshot provider must cover `cam_pos_diff_`,
`pending_reset_`, `last_rotation_timestamp_`, lifecycle/init state and stable CK
object references, with validation and unload/level-exit invalidation. Settings
should remain user preferences rather than silently rewind. No heuristic reset
has been added. Death/transformer transitions and TAS playback are not qualified.

## Reproduce the browser checks

Configure with `-DCAMERA_PORT_PROBE=ON` to build the separate observation-only
CameraPortProbe package; do not ship/install it as a default mod. With Playwright
installed in packaging/web/node_modules, run from the Ballanced root:

```sh
node mod-ports/StaticCamera/tests/qualify-camera.mjs http://localhost:18779 \
  build/mods/static-test build/mods/static/StaticCamera.bwmod \
  build/mods/static/CameraPortProbe.bwmod static
```

The script uses a fresh profile and public import/settings UI. It records
`snapshotCompatible: false` separately from the ordinary-gameplay pass. Override
`CHROME_EXECUTABLE` and `PLAYWRIGHT_MODULE` for other environments. Service workers
are deliberately blocked to isolate the candidate; offline-cache status errors
in those screenshots are harness artifacts, and offline behavior is not tested.
