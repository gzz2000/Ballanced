# DebugUtilities

Bundled by default in BallanceWeb 20261103.3 and later. No manual import is needed.
An older user-installed copy remains on disk but the bundled version takes
precedence; existing configuration is preserved. Cheat mode remains off by default.

The authoritative port source stays in BML Plus's bundled mod directory:
https://github.com/gzz2000/BallanceModLoaderPlus/tree/web-port/mods/DebugUtilities

See `README_WASM.md` there for keys, commands, MIT license, standalone SDK build
and private-state snapshot limitations. This directory intentionally does not
duplicate that source. Package version: `0.3.2-web.1`.

Qualified source commit: `c96b60e24a5f991d4e28df4537077925c85ddd54`.
SDK fingerprint: `bw1-52b52372ca2595105916cb86` (ABI v1 revision 1).
Emscripten: 6.0.13. Package SHA256:
`7b1df6ed2441d074682379b634b3841395040cc7c9ed40e0ac59c173cfa9a184`.

Browser qualification from this checkout:

```sh
node mod-ports/tests/qualify-debugutilities.mjs http://localhost:18766 \
  /tmp/debug-ordinary /path/to/DebugUtilities.bwmod
node mod-ports/tests/qualify-debugutilities.mjs http://localhost:18766 \
  /tmp/debug-tas /path/to/DebugUtilities.bwmod /path/to/HS01.tas tas
```

Uses isolated Chrome profiles, real DOM command/key input, live CK object/array
inspection, PracticeTools snapshots and public install/remove UI. TAS validation
is one reference HS01 completion, not a full-corpus or trajectory-equivalence claim.
