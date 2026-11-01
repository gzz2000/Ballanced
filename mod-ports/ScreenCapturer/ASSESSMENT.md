# ScreenCapturer assessment (not a port)

Upstream ScreenCapturer 0.0.1 by BallanceBug:
https://github.com/Xenapte/MyBMLMods/tree/3718197500d2bc3e88cf20e5c27b377a4be6f22a/ScreenCapturer
The original source was inspected in full; see the upstream repository license.

It handles F8, PNG/JPEG selection, clipboard-only/copy options, timestamped output,
and notification. Actual capture is HWND client pixels through GetDC/BitBlt,
Windows CF_BITMAP clipboard and GDI+ encoders, with a four-thread asio pool.
These are platform services, not ordinary CK rendering calls.

The missing browser service is a safe asynchronous post-present capture request:
renderer-owned WebGL/WebGPU readback, PNG/JPEG encoding, user-data-file output, and
optional clipboard permission/error handling. Calling canvas.toBlob directly from
OnProcess could observe a cleared/incomplete frame. A host service should queue
requests for the correct render boundary and safely cancel/completion-dispatch
across plugin unload. Direct unrestricted DOM snippets are not a stable mod ABI.

No fake capture implementation was packaged. This plugin remains unsupported until
those actual services are designed and tested. Other ports do not require them.
