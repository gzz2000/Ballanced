# BallanceWeb Mod format v1

A `.bwmod` is a gzip stream containing:

1. Eight bytes `42 57 4d 4f 44 31 00 00` (`BWMOD1\0\0`).
2. A little-endian uint32 length of the following UTF-8 JSON manifest.
3. That manifest, without padding.
4. File contents concatenated in manifest `files` order, without padding/trailing bytes.

The container is intentionally independent of ZIP filename/encoding rules. Browser
DecompressionStream and Python gzip are sufficient. HTTP/offline transport remains
part of the site's versioned caching; package installation uses IndexedDB/IDBFS.

Example manifest:
```json
{
  "format": "ballanceweb-mod",
  "formatVersion": 1,
  "id": "WebHello",
  "version": "1.0.0",
  "sdk": "bw1-<exact SDK fingerprint>",
  "abi": {"id": "ballanceweb-cxx-wasm32-v1", "minRevision": 0},
  "entry": "module.wasm",
  "create": "BW_WebHello_Entry",
  "destroy": "BW_WebHello_Exit",
  "requires": [{"id": "AnotherMod", "version": "1.0.0"}],
  "webExports": [],
  "files": [{"path": "module.wasm", "size": 1234, "sha256": "<64 lowercase hex digits>"}]
}
```

`id` identifies the package (ASCII letter then letters/digits/_/-, max64 chars).
It is not necessarily IMod::GetID: the bundled LegacyTASRuntime package registers
the original `Segment` IMod. The SDK helper uses C-identifier IDs and requires an
explicit version. Dependency versions are exact strings; null means presence
only. All dependencies are required, with missing/version conflicts rejected.
Cyclic service dependencies are supported: link every side module, resolve all
symbols, then register factories in catalog order, followed by sorted user packages.
The bundled TAS/Practice pair needs this two-phase linking. Constructors must not
call peer-module functions before the link set has finished loading.

`entry` must identify a WASM v1 Emscripten SIDE_MODULE (`dylink.0`). The exported
C factories use `IMod *(IBML *)` and `void(IMod *)`, same as BML. Names must be
unique across installed packages; the SDK renames BMLEntry/BMLExit automatically.
`webExports` optionally exposes exported functions to the host's Module/ccall;
ordinary BML mods leave it empty. Reserved host names and duplicate bridge names
are rejected. Other C++ symbols follow the Emscripten global namespace/ODR rules.
Avoid privately linking CK, BML, physics or another copy of the engine.

Files must have unique relative forward-slash paths, no empty/dot/dotdot parts,
backslashes, NUL or absolute paths. SHA256 and exact lengths are checked before
linking; limits are128MiB compressed,256MiB unpacked,1MiB JSON,2048 files.
A compiled-module export check rejects absent factories before installation.
SHA256 detects corruption; it is not a signature or proof of authorship.

# Compatibility and lifetime

`sdk` records the SDK build fingerprint; it is not the compatibility version.
New packages also declare `abi: {"id":"ballanceweb-cxx-wasm32-v1",
"minRevision":0}`. A host accepts them when the ABI id matches and its revision
is at least minRevision, regardless of SDK fingerprint. Older packages without
`abi` are accepted only for the current SDK or an explicitly certified legacy
SDK. The first release SDK `bw1-6b30dfe53997de66cc0e56ff` is certified for ABI v1;
its original binaries are retained for regression tests. Unsupported ABI majors,
future minimum revisions and unknown legacy SDKs fail before linking.

ABI v1 compatibility is a maintained contract: preserve existing C++ layouts,
vtable slots, calling conventions, and published host exports/signatures. The
compiler contract is Emscripten6.0.13,wasm32,C++20,Emscripten exceptions,no pthreads,
PIC,ffp-contract=off. `abi/v1.json` records the original public headers, toolchain
and linked exports. SDK generation rejects unreviewed header/toolchain changes;
linking retains original symbols even if current default mods stop using them;
post-link and packaging checks reject removed/changed WASM export signatures.
Header checks are deliberately conservative, including comments: a harmless edit
requires maintainer review, not a new ABI or forced third-party recompilation.

Compatible releases keep the ABI id. New optional services should use separate
extension interfaces/opaque handles and increase revision; do not reorder fields
or append virtual methods to existing interfaces. A genuine incompatible change
requires a new ABI major or a compatibility adapter that preserves the old one.
Do not relabel incompatible layouts with the same ABI id. Run the frozen old
plugin binaries against new hosts; rebuilding tests with new headers would hide
the compatibility failure. Binary compatibility does not certify every mod's
behavior, arbitrary private hooks, or complete-world snapshot support.

A package can only use services available in its declared ABI revision. Native
Windows PE and macOS Mach-O `.bmodp` files are not WASM packages. Do not edit an
incompatible package's manifest to bypass a version check.

BML manager, core BML/NewBallType integrations and the engine remain in the host.
BallanceTAS, LegacyTASRuntime and PracticeTools are dynamically loaded default
packages. No instance of those mods is statically linked into PlayerBrowser.
Native and Node qualification hosts retain their original static linkage.

Installation/removal applies at document restart, not during a live level.
Modules share the main WASM memory/table and are retained until page teardown;
BML still owns each IMod and invokes its normal unload/destructor callbacks.
These are trusted plugins, not isolated sandboxes. Load only code the user trusts.

Defaults come from `mods.json`, versioned with the game and cached offline.
User packages live in `/user/ModLoader/WebMods/<id>.bwmod`, persist in IndexedDB,
and cannot replace default packages. Extracted resources are recreated at
`/web-mods/<id>/` before startup, registered as a BML data path. The SDK exposes
BWMOD_RESOURCE_ROOT for direct resource reads. Write settings/data below `/user`,
not the volatile resource directory. Configs continue under ModLoader/Configs.

Installing a mod does not automatically make its private state snapshot-safe.
Gameplay-affecting timers, callbacks, external references and C++ state need
explicit integration/testing with PracticeTools. Only claim tested behaviors.
