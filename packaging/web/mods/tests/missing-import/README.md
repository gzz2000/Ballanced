# Missing direct-import preflight regression

This is deliberately **not a playable mod**. Build with Emscripten 6.0.13 and the
public SDK, and install only in the qualifier's isolated browser profile:

```
emcmake cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBALLANCE_WEB_SDK=/absolute/path/to/sdk/wasm
cmake --build build
node qualify.mjs http://localhost:18780/ output build/MissingImportProbe.bwmod build/MissingImportProbe.wasm
```

The absent external function is a genuine direct `env` function import; the
qualifier verifies its compiled WASM import descriptor. A global C++ constructor
would write `/user/missing-import-constructor-ran.txt` before
calling it. This reproduces the class of lazy unresolved imports which a GOT-only
finalizer does not reject early enough.

The test uses normal UI import/restart, expects a readable error naming both the
mod and absent interface, checks that the OnLoad marker never executes,
removes the plugin through UI, restarts successfully and verifies that the marker
file is absent. The removal's persistence step would also flush any constructor
file written before failure, so recovery verifies both OnLoad console and constructor file effects.
No injected loader patch, package corruption or host rebuild is used.
