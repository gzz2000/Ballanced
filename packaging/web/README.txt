WebAssembly qualification build
==============================
This directory currently builds a Node-hosted wasm32 Player with the real CK2,
BuildingBlocks/IVP, BML Plus, BallanceTAS and legacy TAS environment linked in.
It is NOT yet a browser distribution. NODERAWFS is for headless reference tests;
the browser build must use a virtual filesystem and a yielding event loop.

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

Next gates, in order:
1. All qualified TAS trajectories and score streams pass in WASM; fast/precise
   arithmetic, world lifetime and mode transition checks pass.
2. WebGPU renderer, display-synchronized presentation, deterministic input/physics
   scheduling, input and audio. Repeat headless tests after integration changes.
3. Offline cache, persistent user data, TAS editing and complete-world snapshots.

Actual browser qualification (still no interactive renderer):
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
implemented. These are not claims of full rasterizer API compatibility. Offline
persistence and whole-world snapshot qualification are separate later gates.
