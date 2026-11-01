# Public SDK bitmap/font acceptance test

Build this standalone .bwmod with the public SDK, then run against a browser
candidate with the font backend. It is a test plugin, not a default gameplay mod.
From the Ballanced root:

```sh
emcmake cmake -S packaging/web/mods/tests/font-probe -B build/font-probe -G Ninja \
  -DBALLANCE_WEB_SDK="$PWD/sdk/wasm" -DCMAKE_BUILD_TYPE=Release
cmake --build build/font-probe
node packaging/web/mods/tests/font-probe/qualify.mjs http://localhost:18780 \
  build/font-probe/results build/font-probe/FontProbe.bwmod
```

Requires Playwright in packaging/web/node_modules. Override PLAYWRIGHT_MODULE
and CHROME_EXECUTABLE if needed. Each run creates a fresh headless browser
context, installs through the public mod UI and restarts. Service workers are
blocked so an older cached candidate cannot contaminate the result; this does
not qualify offline behavior.

Checks:

- Create/copy/delete bitmap with asymmetric full ARGB pattern, including varying
  alpha and different row/channel values: byte-exact roundtrip.
- Create/query/delete 18-point Arial font and draw non-background glyph pixels
  with asymmetric RGB foreground. Bitmap pixels are read through the public
  VxCopyBitmap API, not engine memory offsets or private SDL handles.
- Left/center/right, top/middle/bottom and rectangle clipping.
- UTF-8 arrows and CJK text with actual rasterized pixels. The output PNGs enable
  checking glyph identity; pixel count alone cannot distinguish missing glyphs.
- Six CKSpriteText public Save/Load cycles. Read each chunk's public font
  identifier and verify requested font size18 and weight700 remain stable.

2026-11-01, SDK bw1-52b52372ca2595105916cb86, ABI v1 revision1, candidate18780:
PASS. Roundtrip mismatch0; glyph counts Hello540, arrows292, CJK1116. Images
visually show Hello, ↑↓←→, 中文测试. Horizontal starting x17/100/185, vertical
starting y8/34/61. Saved sizes[18,18,18,18,18,18], weights all700, errors0.
Initial harness tried an unexported HEAPU8 accessor and failed before assertions;
replaced with the probe's exported hex string. No host/SDK export added.
This test does not assert pixel-exact native GDI glyph metrics or platform font
availability; browser fallback glyph shapes can differ across operating systems.

Multiline extension: modes8–10 render two LF-separated rows with top/middle/bottom
alignment. Modes11–12 render three rows at12pt with LF and CRLF and require equal
pixel hashes. Mode13 clips three long rows to a48x40 rectangle. The harness counts
separate foreground-pixel bands, so flattening lines into spaces cannot pass.

Multiline acceptance on final release20261101.14 candidate18781
(version bfd7e2bd2dfe0ba3c2af5d44): PASS. Two-row foreground bands are
[13..29,40..56] (top), [26..42,53..69] (center), [39..55,66..82] (bottom).
Three-row LF and CRLF have identical pixel hash2104636844 and bands
[10..21,27..38,44..55]. Clipped output ends at x63/y47, including the partial
third row. Inspected all six multiline PNGs: correct text, ordering and clipping.
All previous bitmap/font/save-load assertions continue to pass.
