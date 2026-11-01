# Practical porting contract

BallanceWeb uses one wasm32 engine/BML memory and separate Emscripten side modules.
The `.bwmod` is a bounded gzip container with a JSON manifest and SHA256-checked
files. The SDK packer creates it and verifies WASM exports. The website validates
SDK, files, dependencies and entrypoints before registering factories. Default
TAS/Practice modules demonstrate cyclic services: all modules link before any
IMod is constructed. BML itself and core NewBallType support remain in the host.

## Obtain and build against the matching SDK

Use the SDK distributed with the target BallanceWeb release, not a random BML
header checkout. Read its sdk.json and the site's mods.json: ABI ids must match,
and the host revision must meet the package minimum. SDK fingerprints may differ.
An older host with no ABI support still requires its exact SDK.
The initial contract is Emscripten6.0.13,wasm32,C++20,`-fPIC -fexceptions
-ffp-contract=off`,no pthreads. Prefer an existing matching emsdk; do not replace
the user's global compiler. Activate the requested SDK in the build shell.

Fetch the ready-to-use SDK and skill from the project repository:

```sh
git clone --depth 1 --branch web-port --filter=blob:none --sparse \
  https://github.com/gzz2000/Ballanced.git Ballanced
git -C Ballanced sparse-checkout set sdk/wasm skills/ballance-wasm-mod
```

Use `Ballanced/sdk/wasm` as BALLANCE_WEB_SDK. This contains the exported headers,
helper, packer, licenses and example; do not clone/build the full engine or BML
just to compile a mod. Read its README.md for setup and regeneration details.
The original SDK `bw1-6b30dfe53997de66cc0e56ff` (web release 20261101.12) is
explicitly supported by ABI v1, including old binaries without an ABI field.
`web-port` is not a fixed release. Compare the declared ABI/minimum revision
rather than assuming the newest branch works with every older host.
Record `git -C Ballanced rev-parse HEAD` along with sdk.json in test notes.

If the required compiler is unavailable, the official toolchain repository is
https://github.com/emscripten-core/emsdk.git. For the initial SDK, in a local
checkout: `./emsdk install 6.0.13`, `./emsdk activate 6.0.13`, then source
`emsdk_env.sh`. Use the version from the selected SDK for later revisions.
Python 3, CMake 3.20+ and Ninja are also required. First build sdk/wasm/example
with the same commands below, then adapt it to the user's source mod.

ABI v1 (`ballanceweb-cxx-wasm32-v1`) preserves the existing CK/BML layouts,
vtable slots, compiler conventions and exported host services. The packer copies
the SDK ABI/revision into the package's minimum requirement. Compatible host
updates accept older binaries even with a different SDK fingerprint. Future
extensions should add separate/versioned services; do not modify old interfaces
or bypass checks by editing manifest identifiers. Interface compatibility does
not automatically cover private engine hooks or full-world snapshot behavior.

For host/SDK work, `packaging/web/mods/abi/v1.json` is the preserved baseline.
Header/toolchain and post-link signature guards run during builds. Retain old
exports and run frozen binaries in `packaging/web/mods/abi/fixtures/` on the new
host; never rebuild these fixtures with the current SDK. Ordinary mod porting
should use the public SDK and should not alter the host ABI baseline.

A standalone CMakeLists.txt can contain:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyMod LANGUAGES CXX)
include("${BALLANCE_WEB_SDK}/BallanceWebMod.cmake")
ballance_web_mod(MyMod ID MyMod VERSION 1.0.0 SOURCES MyMod.cpp
  RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/texture.png=texture.png")
```

```sh
emcmake cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBALLANCE_WEB_SDK=/absolute/path/to/sdk
cmake --build build-wasm --parallel 8
```

The helper builds a SIDE_MODULE and produces `build-wasm/MyMod.bwmod`. It renames
BMLEntry/BMLExit to unique C symbols, so keep ordinary `MOD_EXPORT` factories in
source. Do not manually add a second set of factories. Use a C-identifier ID;
`REQUIRES OtherMod@1.2.3` declares an exact dependency, `REQUIRES OtherMod` requires
presence only. Do not introduce a dependency unless code/resources require it.
Avoid linking CK2/BML/physics implementation archives into the plugin: doing so
can duplicate engine globals and snapshot state. Only private plugin dependencies
belong in the side module; compile every such object/archive with PIC as well.

If an imported host function is missing, verify the SDK/build options and whether
the API is implemented/exported. Do not silence link failures or replace it with
an unverified stub. An additional host service may require a new host+SDK release.

## Source audit targets

- `windows.h`, MinHook, PE module scans, hardcoded addresses: use BML lifecycle,
  object/behavior graph APIs or supported instance hooks. wasm has no x86 prologue
  to patch. Keep original platform code guarded if still needed.
- Win32 filesystem paths: use `/` or filesystem path joining. Read packaged files
  below `BWMOD_RESOURCE_ROOT` or via BML's registered data paths. Persist configs
  and data under `/user`; `/web-mods/<id>` is reconstructed on each start.
- IMod callbacks: use `override` with the matching SDK signature. BML discovers
  callback overrides using vtables; a near-match can compile and never run.
- Pointer-sized data, calling conventions and casts: use uintptr_t/typed APIs;
  do not copy native64 layouts or PE32 offsets into reconstructed CK objects.
- Busy waits, blocking GUI, native threads/sockets: callbacks run on the browser
  game thread. Return promptly; use host-supported scheduling/async interfaces.
- FPU/x87 manipulation: the strict solver belongs to the engine. Preserve its
  mode and deterministic input/time sequence; do not attempt native CPU control.
- Static initialization: constructors must not call peer-module services before
  the full dynamic link set is ready. BML OnLoad is the normal initialization site.
- Timers/callbacks/object references: unregister appropriately on unload. Private
  gameplay state is not automatically captured by PracticeTools snapshots.

## Validate beyond compilation

1. Build the SDK example first when diagnosing a toolchain/host mismatch.
2. Start a local copy of the matching web release with its included serve.py.
   Install the package in Settings → User data → Plugins; restart to apply.
3. Inspect `/user/ModLoader/ModLoader.log`: confirm the actual mod loaded once,
   required callbacks fire and packaged resources can be read.
4. Exercise the mod's intended UI/input/gameplay; verify native behavior has not
   been accidentally mapped to a different time unit or sampling rate.
5. Restart and verify its configuration persists. Test offline reopening after
   site caching completes. Remove/restart and confirm callbacks disappear.
6. If it affects gameplay or snapshots, test repeated save/restore/overwrite,
   level exit/reentry, timers and references across object destruction. For TAS
   interactions use a known passing tape and compare the relevant trajectory or
   outcome, explicitly recording whether full trajectory equivalence was checked.

Use disposable browser/user profiles for destructive checks and distinct outputs
for concurrent independent tests. Record concise procedures/results, not large
runtime copies. Report a failed scenario as a failure; do not call a mod fully
compatible because it loads once.

## Lessons from the ys168 port batch

See `mod-ports/README.md` in the full repository for reproducible source ports and
feature-level tests. A sparse SDK checkout does not include those examples;
`git sparse-checkout add mod-ports` retrieves them.

- Identify the downloaded binary by SHA256 and the source by commit, version and
  license. Source version identity does not prove a byte-identical native build.
  If upstream publishes no license, retain a pinned fetch/patch recipe instead of
  silently treating its source or binary as redistributable.
- Older BML sources use `m_bml`, BMLAll includes and different const/callback types.
  Use this SDK's `m_BML` and callback signatures. Resolve timer overloads explicitly
  with CKDWORD; convert chrono durations explicitly rather than assuming `.count()`
  is nanoseconds on every standard library.
- Behavior names are exact asset strings, including trailing spaces. PhysicsEditor
  encountered `Tutorial freeze/unfreeze `; verify the graph and all parameters
  before writing. Follow `GetRealSource()` when inputs share parameter chains.
- `GetTime()` is wall time; `GetAbsoluteTime()` is scaled simulation time. Exercise
  pause, speed changes and accelerated TAS before making clock equivalence claims.
- A compiled side module can retain unavailable direct-function imports. Revision1
  adds 31 typed math exports, including `roundf`; the host preflights imports before
  constructors. It does not promise every libc function. Test the real code path,
  not just loading. Never edit a manifest to bypass an unavailable service.
- `BGui::Label` uses game bitmap fonts; `BGui::Text` uses CKSpriteText/Vx fonts.
  String getters alone did not reveal an empty font backend. Inspect real pixels,
  line breaks, Unicode, clipping and repeated Save/Load font-size stability.
- Private caches may disagree with restored engine objects. KeystrokeViewer now
  compares against the actual sprite text instead of trusting its cached string;
  PositionViewer resets its measurement window on clock rewind. These observation
  fixes do not constitute general plugin-private snapshot support. StaticCamera's
  private offset still cannot be rewound by the current snapshot system.
- Audit existing APIs before adding services: CKWaveSound already has transport,
  gain and position, but the current reader is WAV-only. MusicReplacer needs actual
  MP3 decoding/import; ScreenCapturer needs async browser capture/encoding/clipboard.
