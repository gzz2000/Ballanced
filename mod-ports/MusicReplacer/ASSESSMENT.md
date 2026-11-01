# MusicReplacer assessment (not a port)

Downloaded `MusicReplacer.bmodp`, version 1.0.3, original ys168 BML+ archive.
Author's release discussion: https://forum.bcrc.site/d/118 . It describes per-level
MP3 replacement in Music/Level_XX.mp3, crossfades, and a progress overlay; its
published links are binary downloads. No matching public source was located.
No license or permission to redistribute a reconstructed implementation is inferred.

Headless IDA inspection of the actual DLL, without launching GUI IDA:

- 0x10002210: MCI open file as alias `bgm`, fallback `type mpegvideo`, milliseconds
  time format, initial gain zero, then play.
- 0x100026D0: stop/close bgm and clear current-path state.
- 0x10002960: MCI status/mode query, compare with `playing`.
- 0x10002DF0: OnLoad config, registers 200 ms loop-monitor timer; default loop
  fade lead 2000 ms, fade-in step 15, fade-out step 20, volume 80/100.
- 0x10004FC0: monitor playback position/state and perform loop fade/restart.
- 0x100057F0: fade in from zero toward configured volume * 10.
- 0x10005A80: gain ramp clamped 0..1000, 30 ms timer steps, completion callback.

The binary includes static TagLib and calls WinMM mciSendStringA. Hex-Rays inferred
some timer callback/vararg types incorrectly; this is an assessment, not a claim
that all functions and edge cases have been reconstructed.

Existing SDK CKWaveSound/CKSoundReader APIs already cover gain, playback position,
duration, seek and buffers. The present engine WavReader uses dr_wav and does not
decode MP3. A plugin-private MP3 reader/decoder may therefore be sufficient without
changing the audio ABI. Browser import of user music and resource layout still need
qualification. No silent no-op MCI shim and no misleading .bwmod were produced.
