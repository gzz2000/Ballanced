# BML Plus ports for BallanceWeb

This batch starts from the previously downloaded ys168 plugin collection and
records each source revision, binary identity and license in its own directory.
These are optional, separately installed `.bwmod` packages, not default mods.
Use BallanceWeb 20261101.14 or later with ABI `ballanceweb-cxx-wasm32-v1` revision1.
Import packages in Settings → User data → Plugins and restart the game.

| Port | Source version | Tested behavior / remaining scope |
| --- | --- | --- |
| [DebugUtilities](DebugUtilities/README.md) | 0.3.2-web.1 | Bundled BML debug commands and keys, standalone dynamic SDK build; transient private state is not snapshot-integrated |
| PositionViewer | 0.1.0 | Real position/speed HUD, clock rewind after restore, configuration and unload |
| EnergyItemCounter | 0.1.1 (download was 0.1.0) | Level10 collect → restore → recollect → restore, visible count and multiline text |
| KeystrokeViewer | 0.1.0 | Held/released arrows, held input across restore, all-key mode, configuration persistence |
| StaticCamera | 0.1.0 | Follow offset, quarter-turns, camera synchronization, enable/disable; **private camera offset is not snapshot-compatible** |
| RealTimeMeter | 0.1.2 | Actual wall/simulation clocks, pause, level finish, disable, cleanup; private timer rewind not qualified |
| PhysicsEditor | 0.0.4 | Six physics graph parameters, gravity, physics factor, global speed and real movement; changes deliberately affect TAS behavior |
| FogMode | pinned upstream source | Actual colored fog/settings and unload; fetch/patch recipe only, upstream has no declared license |

The six MIT ports include source and licenses and can be built independently.
FogMode was built/tested locally; its upstream source and binary are not included
in the distributable bundle. See its directory for the reproducible recipe.
MusicReplacer and ScreenCapturer directories contain implementation assessments,
not placeholder ports. No compatibility claim is made for untested plugins.

## Build

Activate Emscripten6.0.13 and run from this repository (replace PositionViewer):

```sh
emcmake cmake -S mod-ports/PositionViewer -B build/mods/PositionViewer -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBALLANCE_WEB_SDK="$PWD/sdk/wasm"
cmake --build build/mods/PositionViewer
```

No engine or game-asset copy is needed to compile these source ports. The matching
SDK and porting skill are in `sdk/wasm` and `skills/ballance-wasm-mod`; individual
READMEs describe behavior differences and feature-specific test commands.

## Validation and SDK findings

Fresh Chrome/WebGL2 profiles exercised the six MIT ports together through UI
installation, real Level10 movement, pause/resume, config creation, clean unload,
and a complete offline refresh. HUD3 + RealTimeMeter also completed the reference
HS01 TAS with no fall/death. PhysicsEditor/StaticCamera were deliberately excluded
from that tape test. This is not full-corpus, full-trajectory or arbitrary-plugin
snapshot qualification. Individual ports document narrower test boundaries.

The batch led to these shared fixes:

- ABI v1 revision1 retains 31 additional typed C math exports, including `roundf`.
  It leaves the original v1 headers and export signatures intact.
- Required direct imports are validated before any module constructor executes.
  A negative fixture verifies readable failure and removal/restart recovery.
- The Vx browser backend now implements bitmap/font APIs used by BGui::Text,
  including UTF-8, multiline text, alignment and clipping. Bitmap color channels
  and row orientation are checked with a byte-exact asymmetric ARGB roundtrip.
- CKSpriteText preserves requested font size over repeated Save/Load cycles.
- Frozen original-SDK plugins still load; frozen TAS/Practice modules complete
  HS01 with two captures and four restores on the updated host.

Remaining SDK opportunities are explicit plugin snapshot providers (StaticCamera
private state demonstrates the gap), asynchronous post-render capture/encoding/
clipboard for ScreenCapturer, and MP3 decoding/resource import for MusicReplacer.
The latter can reuse existing CKWaveSound/CKSoundReader transport APIs.

Maintained browser tests are beside the ports and under
`packaging/web/mods/tests/`. Run them in disposable profiles. Short experiment
notes live in the workspace's `docs/experiments/E3970*.txt`; large temporary logs
and screenshots may be retired after recording the result.
