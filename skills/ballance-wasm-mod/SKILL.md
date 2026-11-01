---
name: ballance-wasm-mod
description: Port C++ BML/BML Plus mods to dynamically loadable BallanceWeb WASM packages, using the matching SDK, and validate plugin lifecycle, resources and browser behavior. Use for source-level mod porting or .bwmod build failures.
---

# Port a Ballance mod to WASM

Produce a loadable `.bwmod`, source changes, and a short record of actual tests.
Use the target site's `mods.json` to identify its SDK. The `.bwmod` SDK identifier
must match exactly; changing a manifest to pretend compatibility is not a port.

Read [references/porting.md](references/porting.md) for the build contract,
platform changes, and validation sequence. The matching SDK contains
`BallanceWebMod.cmake`, `pack-mod.py`, `sdk.json` and `include/`; use those tools
rather than inventing a second container or linking another engine copy.

Inspect the mod's source and dependencies first. Separate ordinary BML callbacks
from Windows APIs, fixed-address hooks, x86 code and assumptions about pointers or
object lifetime. Reuse the reconstructed CK/BML APIs where available. If only a
Windows binary exists, explain the need for source or a separately scoped reverse
engineering effort; it cannot simply be recompiled as WASM.

Preserve the original mod's behavior and user-selected scope. Keep native builds
working through narrow platform branches. Do not change physics, timer units or
TAS sequencing merely to make a plugin compile. A successful compilation is not
proof that callbacks, input, files or save/restore work.

Default to installation followed by restarting the game. BallanceWeb does not
promise safe hot unload inside a live level. Document any private gameplay state
that the current snapshot system cannot restore, and test the actual behavior
before claiming compatibility.

For an in-tree Ballanced checkout, the canonical SDK example and package format
are in `packaging/web/mods/`. For a standalone mod, copy the SDK's example or make
a small CMake target using its helper. Record the SDK id, toolchain version,
package checksum, tested scenarios and unresolved limitations. Do not bundle the
proprietary game assets with the mod or SDK.
