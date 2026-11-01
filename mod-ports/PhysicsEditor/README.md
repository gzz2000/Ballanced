# PhysicsEditor for BallanceWeb

Source port of BallanceBug/Xenapte's Physics Editor 0.0.4 (MIT):
https://github.com/Xenapte/BallancePhysicsEditor at
`e50adf718e1a9555a4275a695daf1315405cf7f5`.

It edits the same six original Set Physics Globals behavior graphs, exposes the
three-axis gravity and physics time factor, and sets CKTimeManager's global time
scale. Defaults remain gravity (0,-20,0), physics factor 2, game speed 1. Changing
these values deliberately changes gameplay and invalidates ordinary reference TAS
expectations; this is not a replacement for BML's frame-rate practice slowdown.

Import the .bwmod using Settings > User data > Mods and restart. Settings remain
in the ordinary BML mod configuration. Like upstream, changed physics values are
applied on level/nav transitions; global speed applies on config notification.

Build with Emscripten 6.0.13 and the public SDK:

```
emcmake cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBALLANCE_WEB_SDK=/absolute/path/to/sdk/wasm
cmake --build build
```

Port differences: modern BML const callbacks/member name, WASM factories, explicit
one-frame timer overload, HUD cleanup before engine teardown, validate all six
input sources before applying settings, and resolve shared parameter sources.
Stock game data spells `Tutorial freeze/unfreeze ` with a trailing space; this
port handles both that spelling and the upstream source's spelling. A direct
unmodified source port crashed when dereferencing the missing exact-name match.
Optional read-only WebStatus export is for the included acceptance test and adds
no host ABI requirement.

Validation uses four isolated real browser game sessions: default, gravity Y=-10,
physics factor=1, and global speed=0.5. It reads all six graph parameters, measures
actual absolute engine clock per tick, waits for ball-navigation activation before
holding a real direction key, and checks normal shutdown releases the HUD.

Private config/cache and queued mod timers are not automatically snapshot data.
PracticeTools restore with physics settings changed between capture and restore,
modified custom behavior graphs, all original levels and multi-level lifecycle
have not been qualified. Installation/restart is supported; live unload mid-level
is not promised. Audio playback speed is not pitch/time-stretched by CK time scale.
