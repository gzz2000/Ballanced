# FogMode for BallanceWeb

Source-level port recipe for Ghomist's FogMode:
https://github.com/Ghomist/FogMode4Ballance/tree/2de9635c2a7bc91ca610d8e9a43d658b0696dc22 .
The downloaded FogMode.bmodp identifies Ghomist and BML 0.3.10. Source and binary
strings agree on settings/behavior, but exact binary/source identity is not
claimed. Upstream does not provide an independent mod semver: 0.3.10-web.1 is
this package's port version. Full identities are in provenance.json.

Upstream has **no declared source license**. This directory contains only our
pinned fetch/transform/build recipe; it does not vendor upstream source. Source
is fetched into the build directory and SHA-256 checked. Obtain author permission
before publishing the compiled port or redistributing upstream source; local
compatibility experiments do not establish redistribution permission.

## Build

From Ballanced, with Emscripten 6.0.13 and the public SDK:

```sh
emcmake cmake -S mod-ports/FogMode -B build/mods/fog -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBALLANCE_WEB_SDK="$PWD/sdk/wasm"
cmake --build build/mods/fog
```

Optionally set `-DFOGMODE_UPSTREAM=/path/to/FogMode4Ballance` to read the same
hash-checked files from a local checkout instead of the network. Output:
`build/mods/fog/FogMode.bwmod` (ABI v1, minimum revision 1). Import in the browser
mod manager and restart. Fog is disabled by default. Configure it through BML
Mod Options → Fog Mode. Original preset/RGB/hex colors, linear fog and far-plane
behavior are preserved. The recipe adapts BML names/types, exports, sscanf_s,
and protects an empty color string. It does not replace the rendering algorithm.

## Qualification

Candidate revision-1 Chrome/WebGL2 passed install/restart, Level 1, actual BML
config edits (enable, start=5, end=20, color=cyan), visible cyan fog, engine state
and far-plane=32, normal exit and removal/restart. The network-only fetch path
also passed. To repeat the shared browser harness, build StaticCamera with
`-DCAMERA_PORT_PROBE=ON`, then run its tests/qualify-camera.mjs with this package,
the observer package and final argument `fog` (see that port's README).

The upstream README already warns of transparent-texture artifacts with colored
fog; this port preserves that behavior. We did not qualify custom maps, every
color parser case, cross-level texture restoration, offline play, TAS or practice
snapshots. No new SDK service was needed for ordinary fog operation. Plugin-owned
private state is not covered by the engine's existing snapshot provider.
