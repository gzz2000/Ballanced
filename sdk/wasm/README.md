# BallanceWeb WASM Mod SDK

This directory is the ready-to-use SDK for source-level BML/BML Plus mod ports.
It contains public headers, the CMake helper, packer, example and license notices.
No engine build, submodule checkout or proprietary game data is needed to build
a mod. Running it still requires a compatible BallanceWeb installation.

## Get the SDK and porting skill

```sh
git clone --depth 1 --branch web-port --filter=blob:none --sparse \
  https://github.com/gzz2000/Ballanced.git Ballanced
git -C Ballanced sparse-checkout set sdk/wasm skills/ballance-wasm-mod
```

The SDK is `Ballanced/sdk/wasm`; the reusable Codex skill is
`Ballanced/skills/ballance-wasm-mod`. Install that skill directory under your
`~/.codex/skills/` directory and use `$ballance-wasm-mod` with the mod source.

Read `sdk.json` and the target site's `mods.json` before building. The ABI id must
match and the host revision must meet the package's minimum revision. SDK ids
identify builds; compatible updates can have different SDK ids. `web-port` moves forward; it is not a permanent release pin. Record the
full Ballanced Git commit and SDK id used for each distributed mod. For an older
host, use its matching SDK ZIP or the matching historical repository revision.
The original SDK `bw1-6b30dfe53997de66cc0e56ff` (web release 20261101.12) is
explicitly supported by ABI v1. Hosts before the ABI-aware update still require
their exact SDK id.

## Build the example or a mod

Requirements: Python 3, CMake 3.20+, Ninja, and the Emscripten version in sdk.json.
The initial SDK requires Emscripten 6.0.13. If not already installed, use a local
checkout of the official toolchain (do not overwrite a shared compiler setup):

```sh
git clone https://github.com/emscripten-core/emsdk.git emsdk
./emsdk/emsdk install 6.0.13
./emsdk/emsdk activate 6.0.13
. ./emsdk/emsdk_env.sh
```

With the example checkout above, from its parent directory:

```sh
emcmake cmake -S Ballanced/sdk/wasm/example -B mod-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBALLANCE_WEB_SDK="$PWD/Ballanced/sdk/wasm"
cmake --build mod-build
```

Import `mod-build/WebHello.bwmod` through Settings → User data → Plugins, then
restart. OnLoad reads its packaged message.txt; OnPreStartMenu and OnUnload log
observable callbacks. Copy/adapt example/ for your own source mod. Native .bmodp
binaries cannot be loaded as WASM. See FORMAT.md for dependencies, resource paths,
lifetime and compatibility limits.

## Backward compatibility

ABI-aware loading starts with web release 20261101.13.

ABI `ballanceweb-cxx-wasm32-v1` promises to preserve its published C++ interfaces
and host services across compatible host updates. Packages declare an ABI id and
minimum revision; SDK fingerprints are provenance, not an equality gate. Legacy
packages from the original SDK are also supported without rebuilding.

The host freezes public layout headers/compiler conventions and retains previous
exports. Build/packaging checks reject incompatible headers or WASM signatures.
Regression fixtures use original .bwmod binaries, including the old TAS plugins.
A future incompatible layout/toolchain needs a new ABI major or an adapter; a
normal website update must not silently invalidate existing v1 plugins. New
optional APIs should extend separate interfaces and raise the revision.

Revision 1 adds 31 C math exports (including `roundf`) without changing v1 public
headers or existing signatures. Extension manifests live in `abi/v1-r*.json`;
`v1.json` remains the immutable baseline. The host validates required imports
before running side-module constructors and reports missing services by name.
The revision-1 host also implements browser bitmap fonts used by BGui::Text,
including explicit multiline text and stable font size across snapshot restores.
It does not export every libc function or capture arbitrary plugin-private state.

This protects supported interfaces; it does not make undefined behavior, private
engine assumptions, or arbitrary plugin snapshot state automatically compatible.
Do not rewrite manifest ABI fields to conceal an actual mismatch.

## Maintainer regeneration

Canonical generator/helper sources are in `packaging/web/mods/`. This committed
SDK is a distribution snapshot; do not edit its generated headers independently.
In a full checkout with CK2/VxMath submodules and the BML source revision pinned
by `packaging/web/dependencies.json`, regenerate using:

```sh
python3 packaging/web/mods/build-sdk.py --engine Source \
  --bml /path/to/BallanceModLoaderPlus --output sdk/wasm
```

Publish the matching host, SDK and source pins together whenever the contract
changes. No private game assets belong in this directory.
