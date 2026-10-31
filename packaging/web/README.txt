Ballanced WebAssembly / WebGPU
=============================
One shared engine builds three hosts: Node (Player.js), headless browser Worker
(PlayerWeb.mjs), and interactive WebGPU (PlayerBrowser.mjs). CK2, BuildingBlocks /
IVP, BML Plus, BallanceTAS, LegacyTASRuntime and PracticeTools are linked statically.
Node uses NODERAWFS for isolated reference tests. The interactive host uses MEMFS,
IDBFS user persistence and a yielding requestAnimationFrame loop.

Validated toolchain: Emscripten 6.0.13, SDL 3.4.16, CMake/Ninja, Node 24.
Use official emsdk install/activate 6.0.13. Keep -ffp-contract=off. The precise
solver runs the existing SoftFloat/x87 implementation; the fast solver uses
native WASM floating-point. Neither solver substitutes a different physics world.

Build (paths are placeholders for your local checkouts):
  python3 packaging/web/build-headless.py --emsdk /path/to/emsdk \
    --sdl-source /path/to/SDL3-3.4.16 --bml-source /path/to/BallanceModLoaderPlus \
    --tas-source /path/to/BallanceTAS --build /path/to/build/wasm --jobs 12
Optional --fex-source reuses an existing checkout of the pinned FEX dependencies.
BML and TAS must use their matching web-port commits. The core repositories
remain Ballanced submodules; initialize submodules recursively before building.

The build runs arithmetic, native member-pointer/vtable, static-mod registry,
static engine API and collision/checkpoint tests. Collision checkpoint tests run
both fast and precise modes. These are not full-game snapshot acceptance tests.

Headless execution needs your legally obtained, unmodified game assets:
  BALLANCE_HEADLESS=1 BALLANCE_AUTOMATE_QUIT_AFTER_MS=5000 \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  node /path/to/build/wasm/engine/Source/Player/src/Player.js \
    --root-path /path/to/game --cmo /path/to/game/base.cmo \
    --user-data-path /path/to/isolated-user --rasterizer null

TAS regression:
  python3 packaging/web/validate-headless.py --player /path/to/Player.js \
    --game /path/to/game --template-user /path/to/clean-user \
    --manifest /path/to/corpus.json --output /path/to/new-results --jobs 6
The clean-user template supplies Player.ini and Database.tdb. Use null rasterizer,
LegacyTASSort=1, SkipOpening=1, ApplyHotfix=1 and Language=1 in Player.ini.
The manifest is {"cases": [...]} with each case supplying:
  id, level (e.g. Level_02), tape, tape_sha256, reference,
  reference_sha256, score (original gameplay-score CSV).
All paths in that manifest are absolute. Recordings and proprietary game assets
are not bundled in this repository. Original reference traces are BALTRCE format
version 1 or 2, 344 bytes per row. Use the same qualified recording environment.
The runner hashes candidates and inputs, compares all captured fields and score
rows without clock offsets, checks finish/death/normal unload, and saves compact
JSON results. Successful large scratch files are retired unless --keep-traces.
A passing reference corpus does not qualify other recording environments,
full-world save/restore, browser graphics, browser scheduling or offline storage.

Implementation and qualification order:
1. All qualified TAS trajectories and score streams pass in WASM; fast/precise
   arithmetic, world lifetime and mode transition checks pass.
2. WebGPU renderer, display-synchronized presentation, deterministic input/physics
   scheduling, input and audio. Repeat headless tests after integration changes.
3. Offline cache, persistent user data, TAS editing and complete-world snapshots.

Headless browser Worker qualification:
  cmake --build /path/to/build/wasm/engine --target PlayerWeb --parallel 12
  npm ci --prefix packaging/web
  node packaging/web/qualify-browser.mjs /path/to/PlayerWeb.mjs /path/to/game \
    /path/to/clean-user /path/to/corpus.json CASE_ID /path/to/new-results
Set CHROME_PATH to the Chrome executable outside the default macOS location.
The harness creates a fresh disposable browser profile and a loopback-only HTTP
server. Assets load into a dedicated Worker's MEMFS; input/reference hashes and
candidate bytes are preserved for comparison. Environment variables must be set
in preRun, before C++ constructors cache libc's environment. The Worker currently
runs the blocking qualification loop; it is not the interactive game host.
review-browser.py applies exactly the same trace/score gates as the Node runner.

Interactive WebGPU prototype (stage 3)
-------------------------------------
Pass --webgpu to build-headless.py to enable CKWebGpuRasterizer and PlayerBrowser.
It reuses CKFFPLib and its existing fixed-function uniform/shader interface.
Generate WGSL from the checked-in native SPIR-V pack with:
  python3 Source/RenderEngine/src/CKRasterizer/CKWebGpuRasterizer/shaders/compile_web_shaders.py \
    --tint /path/to/tint-with-SPIRV-reader \
    --spirv-grammar /path/to/spirv.core.grammar.json --output /path/to/generated
Validated Tint source: Dawn 97ae1b10ac9d579f7e6b790ce7fc4faa1041f820.
Build with TINT_BUILD_SPV_READER=ON; the prebuilt release Tint may omit it.
The generator verifies native shader hashes and records compiler/output hashes.
WGSL has no ImageQueryLod: the pass lowers the used unclamped LOD component to
texture-size/derivative math; unexpected query consumers are rejected.

  node packaging/web/qualify-renderer.mjs /path/to/generated/shaders.json result.json
  node packaging/web/qualify-browser.mjs /path/to/PlayerBrowser.mjs /path/to/game \
    /path/to/clean-user /path/to/corpus.json CASE_ID /path/to/new-results \
    /path/to/generated/shaders.json
The second command applies the same original-Windows trace and score gate while
presenting real WebGPU frames. The standard-A corpus passed all 31 cases / 964653
eligible rows. Pixel tests cover full/partial clears, repeated resize and 1x/4x
MSAA. Actual browser tests cover keyboard navigation, movement, pause, mouse hit
testing after resize, DPR2 and nonzero audio samples. Presentation follows rAF;
fixed simulation steps consume timestamped input independently of display rate.

Known prototype limits: texture render targets and engine readback APIs are not
implemented/advertised. FXAA, sharpening and render-scale options are not applied;
4x MSAA and physical HiDPI rendering are supported. Volume mip generation is not
implemented. These are not claims of full rasterizer API compatibility.

Offline browser distribution (stage 4)
--------------------------------------
  python3 packaging/web/package-browser.py --player /path/to/PlayerBrowser.mjs \
    --shaders /path/to/generated/shaders.json --game /path/to/game \
    --template-user /path/to/clean-user --bml-source /path/to/BallanceModLoaderPlus \
    --output /path/to/new-site
  python3 -m http.server 18766 --bind 127.0.0.1 --directory /path/to/new-site
Open http://127.0.0.1:18766 in a WebGPU browser. A remote deployment needs HTTPS.
Use a stable hostname/port: browser data belongs to that origin. file:// cannot
provide WebGPU/offline service worker support. The output includes your game
assets (~162 MB in our fixture); proprietary assets are not committed here.

The page waits for a user gesture to initialize GPU/audio. Arrows move the ball;
Enter/Esc navigate the game. Rendering follows display refresh independently of
simulation ticks. Normal play uses WASM native floating point; record playback
selects the software x87 solver and returns to fast mode when TAS ends.
F5/F8 capture/restore one in-memory level snapshot; subsequent saves replace it.
Snapshots include game/physics/animation/timer state and, during record playback,
the playback cursor. They expire on level exit/reset/load or TAS shutdown. They
are not disk saves, nor Lua VM continuation snapshots: unsupported TAS operations
are rejected rather than restored partially. GPU resources remain outside saves.

Import .nmo and .tas through the page. Files go to /user/ModLoader/Maps and TAS.
BML's own menus remain available. Windows binary .bmodp mods cannot be loaded as
WASM; native plugins must be ported and linked into the build. User settings,
Database.tdb, maps and recordings live in IndexedDB, never in the cached assets.
Automatic synchronization runs every ten seconds and on page backgrounding;
use "立即保存数据" or "退出并保存" before closing, and download important files for
backup. Browser eviction/clearing site data can remove both caches and saves.

The service worker hashes every asset before activating a complete new version.
It does not force-replace a running version. Close all tabs of the origin to
activate a pending upgrade. Once the page reports its offline cache ready, the
same URL works offline. Game assets and runtime are versioned together; user data
is separate and survives runtime/cache updates.

The initial TAS editor works in the menu: open a recording, inspect/edit an
inclusive zero-based sample range, undo one change, save a named copy, select a
level and play it. Editing preserves every original float delta unless explicitly
changed; the parser/writer is the C++ TAS implementation. The Enter bit is retained
for file compatibility; legacy playback does not inject it. This is a sample
editor, not a full timeline UI or a Lua/script debugger.

Acceptance tools:
  BALLANCE_TEST_EDITOR=1 BALLANCE_TEST_PLAY=1 BALLANCE_TEST_FAST_CHECKPOINTS=1 \
    node packaging/web/qualify-offline.mjs /path/to/site result.json map.nmo hs01.tas
This exercises editor apply/undo/write, independent decompressed-record equality,
UI-triggered strict playback with repeated snapshots to completion, offline
reload, persisted imports and normal fast-mode hotkey snapshots. Use a qualified
level-1 tape; the play option selects level 1.

  python3 packaging/web/qualify-snapshots.py --player /path/to/Player.js \
    --game /path/to/game --template-user /path/to/clean-user \
    --manifest /path/to/corpus.json --output /path/to/new-results --jobs 8
Optional --case CASE_ID --sequence s6500,r6900,r6900 exercises explicit cursors.
--web-checkpoints goes through the public browser checkpoint command implementation,
including its own cursor rewind; the checker does not perform a second rewind.
With no explicit sequence, each case captures at three eligible trajectory points,
restores twice at each and compares all revisited samples/scores against a separate
uninterrupted control. The control must independently pass the original Windows
gate. Collision-graph and semantic scene restore audits are also required.
For real GPU snapshot cases append SEQUENCE CONTROL_RESULT_DIR to qualify-browser.mjs
(after shaders.json and webgpu.mjs); BALLANCE_TEST_WEB_CHECKPOINTS=1 selects the
same public command path. Each case requires separate output/user directories.

Qualified locally in Chrome 156 / Apple Metal WebGPU: 31/31 standard-A recordings,
964653 original eligible samples, and all 31 repeated snapshot sequences in both
Node WASM and real WebGPU. This covers the qualified corpus, not every possible
map/mod/scene state. Other browser engines and non-Apple GPUs remain unqualified.

Parallel browser corpus runner (freezes JS/WASM/WGSL and uses fresh profiles):
  python3 packaging/web/qualify-browser-corpus.py --player /path/to/PlayerBrowser.mjs \
    --game /path/to/game --template-user /path/to/clean-user \
    --manifest /path/to/corpus.json --shaders /path/to/shaders.json \
    --output /path/to/new-results --jobs 4 --snapshots --web-checkpoints
Omit --snapshots for controls only, or add --case CASE_ID for an isolated case.
Compact results survive; successful large traces are removed unless --keep-traces.
