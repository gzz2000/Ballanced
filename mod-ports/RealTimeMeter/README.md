# RealTimeMeter for BallanceWeb

Source port of BallanceBug/Xenapte's Real Time Meter 0.1.2, MIT licensed.
Upstream: https://github.com/Xenapte/BallanceRealTimeMeter
Commit: `73dbf4861e87d6ad1f4026d7cb6d50f366672fd5`.

Displays original Ingame and World elapsed clocks. The world clock measures actual
elapsed monotonic time, including time in the pause menu. Ingame time accumulates
engine deltas. Browser suspension/background scheduling can affect their relation.
This intentionally preserves upstream semantics, rather than replacing SR Timer.

Import `RealTimeMeter.bwmod` in Settings > User data > Mods, then restart. The
original Enabled and StreakRun options remain in the BML plugin configuration.
The plugin uses game bitmap-font labels, requiring no OS font or DOM service.

Port changes: BML 0.3.14 const callback types/member names, portable factories,
explicit chrono nanosecond conversion (no assumption about steady_clock's native
tick unit), and release the label during OnUnload. Read-only optional diagnostic
exports support the checked-in browser acceptance scripts; no new host ABI.

Build using Emscripten 6.0.13 and the BallanceWeb SDK:

```
emcmake cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBALLANCE_WEB_SDK=/absolute/path/to/sdk/wasm
cmake --build build
```

Acceptance: actual level start, visible running HUD, clock/wall agreement, pause,
resume, menu exit, disabled config persistence, disabled gameplay, clean unload.
A separate real HS01 TAS completed with accelerated simulation: Ingame about
92.588 seconds, World about 6.713 seconds, displayed clocks stopped at finish.
Scripts take URL/output/package and (for qualify-finish) a reference TAS filename.
Their Playwright import expects this repository's packaging/web dependencies.

Not qualified: multi-level StreakRun; private timer state across PracticeTools
save/restore. The latter is not an engine-snapshot guarantee: these C++ clocks and
streak bookkeeping are not automatically included in saved world state.
