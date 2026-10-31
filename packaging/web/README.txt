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
  python3 packaging/web/serve.py --port 18766 --bind 127.0.0.1 --directory /path/to/new-site
Open http://127.0.0.1:18766 in a WebGPU browser. A remote deployment needs HTTPS.
Use a stable hostname/port: browser data belongs to that origin. file:// cannot
provide WebGPU/offline service worker support. The output includes your game
assets (~162 MB in our fixture); proprietary assets are not committed here.
The included serve.py negotiates gzip (only when smaller), caches compressed file
revisions outside the served tree, and supplies Content-Length, Vary and ETag.
It keeps the same loopback URL for SSH forwarding. fetch/cache integrity checks
operate on decoded bytes. A ZIP download alone does not enable HTTP compression.
The offline worker first obtains encoded lengths with HEAD, then reports completed
and verified GET response-body bytes, excluding HTTP headers/SSH framing. Progress
updates per completed file; if wire lengths are unavailable it falls back to file
counts. These transfer bytes are not browser storage quota measurements.

The page installs/verifies its offline resources first, showing encoded-byte
progress before loading the engine; startup then reads the installed cache rather
than downloading the assets in parallel a second time. The game starts
automatically; audio resumes on the first real click or
keypress if autoplay is blocked. After orderly exit and IDBFS synchronization, a
centered restart button reloads the cached runtime with saved user data. A failed
exit save is retried before restart so it cannot silently discard dirty data.
The settings sidebar has Display, User Data and TAS Editor tabs; it is hidden by
default. AA lives in Display. User Data has folder navigation, search, sizes and
file downloads. Desktop layout resizes the game beside it; narrow layouts overlay
it. There are no host-level quit/snapshot buttons or hardcoded checkpoint keys.
The former browser-default F5/F8 pair migrates to A/Z once; other bindings remain
unchanged. Subsequent bindings are owned entirely by PracticeTools configuration.
Arrows move the ball; Enter/Esc navigate the game. Rendering follows display refresh independently of
simulation ticks. Normal play uses WASM native floating point; record playback
selects the software x87 solver and returns to fast mode when TAS ends.
PracticeTools defaults A/Z capture/restore one in-memory level snapshot; subsequent saves replace it.
Snapshots include game/physics/animation/timer state and, during record playback,
the playback cursor. They expire on level exit/reset/load or TAS shutdown. They
are not disk saves, nor Lua VM continuation snapshots: unsupported TAS operations
are rejected rather than restored partially. GPU resources remain outside saves.

Import .nmo and .tas through the page. Files go to /user/ModLoader/Maps and TAS.
BML's own menus remain available. Windows binary .bmodp mods cannot be loaded as
WASM; native plugins must be ported and linked into the build. User settings,
Database.tdb, maps and recordings live in IndexedDB, never in the cached assets.
Automatic synchronization runs every ten seconds and on page backgrounding;
use "立即保存" or the game Exit menu before closing, and download important files for
backup. Browser eviction/clearing site data can remove both caches and saves.

The service worker hashes every asset before activating a complete new version.
Existing games retain their release; a refresh starts the verified new version.
No cache clearing is needed. Once the page reports its offline cache ready, the
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

Responsive browser viewport
---------------------------
The canvas now follows the available browser layout automatically, including
wrapped toolbars and pixel-density changes. Resize does not focus the canvas
while typing in forms. Presentation uses the full viewport and derives horizontal
FOV from each authored camera: wider screens preserve vertical FOV, narrower
screens preserve horizontal FOV and reveal more vertically. Menus/HUD use a
centered 4:3 rectangle shared by drawing and picking; camera objects and physics
are unchanged.
The policy is enabled by PlayerBrowser only; native contexts retain their default.
  node packaging/web/qualify-responsive.mjs /path/to/site /path/to/new-results
This checks compositor pixels at both edges, submitted geometry projection,
repeated/narrow layouts,1x/2x buffers, mouse navigation, focus and idle resize calls.
The legacy sky effect has its own intentional FOV distortion. CDP density changes
need an explicit simulated media notification because CDP emits no browser event.
BALLANCE_TEST_RESIZE=1 with qualify-browser-corpus.py exercises repeated drawable
resizes during reference playback and (with --snapshots) cursor/world restoration.
Latest resize-specific gate: HS02_5885 control +3captures/6restores passed; this is
one additional case, not a claim that the full corpus was rerun for this patch.

Automatic startup/sidebar qualification (2026-10-31):
Chrome 156 with document-user-activation-required starts real WebGPU frames with
no user activation; suspended audio resumes on the first key. The normal exit
button saves IDBFS and shows a geometrically centered restart button. Two offline
restarts retain imported maps, TAS and a sentinel; editor edit/undo/save preserves
the decompressed recording. Responsive qualification also checks desktop sidebar
space reservation, Esc closing, narrow-screen overlay, DPR changes and no idle
resize loop. This host-only iteration did not rerun the full TAS corpus.

Status stays in the toolbar; initial download progress is centered over the game
viewport until startup completes. The canvas fills
all remaining viewport space without outer padding; the right tools sidebar
retains its desktop split/narrow overlay behavior. serve.py defaults to loopback
port18766; retain that origin when updating so IndexedDB data stays accessible.

WebGPU submission optimization (2026-10-31):
Reusable staging arenas batch upload calls at each flush; command submission and
subsequent writes share queue order, with no CPU/GPU waits. GPU resources are not
part of saved worlds. Vertex attribute mapping is resolved once per draw, before
the per-vertex copy loop. Pixel/resize and HS02 strict reference/checkpoint tests
pass. Run profile-browser.mjs URL result.json for disposable real-level CPU/audio
measurements; do not run competing browser validations during comparative timing.
The probe compares idle, held arrow with OS-style repeat, and rapid taps. It logs
Chrome process CPU time, main-thread sampled stacks and audio callback gaps/output
peaks; it does not prove physical speaker output. The reported M4 Chrome held-key
audio freeze was not reproduced on the local M5 Pro Chrome156 environment.
Navigation keys explicitly suppress browser defaults on repeats while all SDL
key events and held-key sampling remain intact.

Browser overlay defaults and compact vertex streams (2026-11-01):
Segment General/Enabled? and BallanceTAS OSD/ShowOSD default off for Wasm.
The host migrates existing profiles once (.web-overlays-v1), then preserves all
subsequent choices from the plugin menu. ShowBallPanel and ShowInputPanel remain
individual controls beneath the TAS OSD master switch. Native defaults unchanged.
Normal play selects fast physics; an active/pending/paused TAS timeline selects
strict x87. These presentation changes do not change simulation or render timing.

Varying attributes use a compact stream; missing attributes use one instance-rate
record of the shader's exact default bits. Wasm views are copied immediately into
staging arenas, removing the intermediate per-draw JS slice. Pipeline cache keys
include the attribute mask/stride. qualify-vertex-packing.mjs URL result.json
replays actual menu/level frames using dense vs compact layouts and compares GPU
readback pixels at 1x/4xMSAA. E3927: zero different bytes in four comparisons.
E3928 isolated first-level Chrome156/M5Pro/60Hz/DPR2: total process CPU about42-43%
to37-39% (100%=onecore); main-thread task25-27% to20%. Upload bytes/frame fall from
3.91-4.71MB to1.06-1.32MB. Includes overlay default changes and profiler overhead;
not an absolute comparison with native or a measurement of the user's M4 setup.

Atomic cached-page upgrades (2026-11-01):
HTML/CSS/JS/Wasm/assets are all included in the version manifest. The previous
worker stayed waiting while an old page was open; a refresh could keep serving
the old HTML indefinitely. A fully verified installation now activates directly.
Per-document cache pins in a separate CacheStorage metadata cache keep existing
games on their own release, even after worker eviction; the next navigation gets
the new release. Closed document pins/unused old caches are collected at activation.
IndexedDB is never cleared. A failed checksum/install keeps the active release.

The host explicitly checks for an update on each load (updateViaCache:none), waits
for installation, compares the document stamp with the active worker and reloads
before engine startup if needed. Already-running upgraded hosts display an Update
and Restart button, which persists user files before reload. Legacy hosts that
predate this protocol need a refresh after the first new installation completes,
but no cache clearing or closing every tab. Later versions update with one refresh.
Qualification: node qualify-updates.mjs LEGACY_SITE CANDIDATE_SITE result.json
uses one fresh profile across versions, a retained old tab, a worker restart,
offline reload, a deliberately corrupt update and subsequent old-cache collection.

Responsive HUD and visual record editing (2026-11-01):
CKRenderEngine shares the authored 4:3 UI rectangle with both 2D roots;
CKBuildingBlocks screen-relative text uses that root extent. Nested score/life
HUD and menu glyphs now scale together without narrowing the 3D viewport.
qualify-hud.mjs URL OUTPUT captures actual paused-level draw packets, compares
width-only translation, return-to-4:3, and clicks the resized Options button.

BallanceWeb by gzz2000 shows the actual fast/strict solver in its header. TAS
status is hidden while idle, shows sample progress while playing, and can stop
active or pending playback. Browser playback defaults to one pass; EOF returns
to fast mode rather than rearming the tape. New defaults migrate once.
The editor uses a virtualized sample/key grid with selection, paint strokes,
column/range fill, per-sample timing, one-step undo and .tas copy saving. One
stroke is one undo transaction; other key bits, legacy flags and original timing
are preserved. Enter is a legacy reserved bit, not a consumed replay input.
Editing is allowed only at the menu; the table is read-only during a level.
qualify-tas-editor.mjs URL OUTPUT TAPE checks real pointer/keyboard editing,
bit-exact undo/file roundtrip, long-tape scrolling, EOF, active/pending stop.

profile-checkpoints.mjs URL CORPUS OUTPUT CASE... measures net mallinfo live bytes
at the checkpoint boundary, separately from Wasm heap reservation. Optional
BALLANCE_PROFILE_BATCH=64 uses a disposable host override for catch-up profiling;
normal gameplay remains unchanged. E3947: levels1/2/10 first snapshot retained
34.83/36.87/36.87MiB in both fast/strict modes. Later overwrites retain a 3-4MiB
CK-value scratch bank. E3948 isolated HS02: capture105-113ms, restore373-470ms,
2048 strict samples in536ms with rendering once per64-sample batch (Chrome156,
M5Pro). These are net allocations, not a serialized payload or measurements of
multiple simultaneous snapshots. No automatic checkpoint index was implemented.
Current ownership/exclusion tracks one snapshot plus scratch bank; a future bank
must exclude every retained snapshot's CK storage, bound memory, invalidate all
checkpoints at/after the earliest edited sample, and prove rewind/replay parity.

Sharing maps and TAS recordings
------------------------------
Links use a client-side fragment; construct parameters with URLSearchParams so
remote URLs containing '&' or other delimiters remain intact:
  #!action=map&map=<encoded HTTP(S) .nmo URL>
  #!action=tas&level=1&record=<encoded HTTP(S) .tas URL>
  #!action=tas&map=<encoded .nmo URL>&record=<encoded .tas URL>
The page asks for confirmation before fetching any shared asset. GitHub /raw/
links are rewritten to raw.githubusercontent.com; other hosts must allow CORS.
Downloaded files go to /user/ModLoader/Maps and /user/ModLoader/TAS in IDBFS.
Identical bytes reuse the original or a numbered variant; differing files get
2, 3, ... before the extension. Existing files are never overwritten. A link
opened during gameplay persists data and starts a fresh engine session, retaining
that link's confirmation for up to ten minutes. Cancellation aborts downloads.
Limits: 128MiB per download; 64MiB declared uncompressed legacy TAS data.

qualify-share-links.mjs URL OUTPUT TAS_RESOURCE_ROOT exercises all three link
forms with real public files, consent, deduplication and persisted reload.
qualify-share-errors.mjs URL OUTPUT covers HTTP/CORS/content failures and aborts.
qualify-share-playback.mjs URL OUTPUT [builtin|custom] checks complete playback.
qualify-custom-corpus.mjs URL MANIFEST OUTPUT WORKSPACE tests independent map/tape
pairs with three isolated browser contexts. Its manifest is an array of objects
with name, scope, tape and maps fields, paths relative to WORKSPACE. A finished
recording is distinct from LevelFinish, especially for partial-sector demos.

2026-11-01 sharing build: HS01_4740_1.30.344_264_口语大赛_onlyforyou
finishes without falls. Thirteen custom-map pairs were exercised in WebGPU;
山路十八弯, 泅溺幽海, 秋名道木球版 and 秋名道石球版 finish without falls.
210522 and 镜界空轨 still fail; partial-sector recordings are not certified.
210522's late fall reproduces both in real time and with render batching.
Exact published environment-A qualification and trajectory repair are PAUSED at
the user's request. This release adds sharing, not a custom-TAS fidelity fix.

Offline transfer packs (2026-11-01.5)
-----------------------------------
The offline installer no longer probes every resource with HEAD. The packager
creates content-addressed offline/*.pack files containing explicit gzip streams;
uncompressed groups are bounded to16MiB except a single larger source file.
Packed resources include game assets, Wasm and shaders. Eight small shell files
are fetched directly. This build uses11 packs totaling68,869,357 compressed bytes.
The central progress meter reports compressed resource payload bytes as chunks
arrive, independently of HTTP Content-Length or CDN gzip/brotli negotiation.
Small shell requests and HTTP headers are not part of this resource-pack total.
Each pack and each unpacked file is SHA256 checked before activation. CacheStorage
retains individual runtime responses, so the engine reads no pack format and old
running tabs keep their pinned version. Failed parallel installs settle all writes
before deleting their partial cache. Loose files remain in the distribution for
non-Service-Worker diagnostic/fallback hosts; this increases deployment ZIP size
but normal offline installation fetches only the packs, not those loose copies.
qualify-cache-progress.mjs SITE RESULT simulates delayed chunked pack responses,
asserts zero HEAD/loose-asset network requests and progress before the first pack
finishes, then reopens offline. qualify-updates also corrupts a pack to verify rollback.
