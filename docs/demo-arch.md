Hactar demo architecture
========================

In the short run, we want to able to demonstrate PTT functionality across Hactar
devices and a demo app running on a mobile / desktop device.  It would be good
to align our work on this short-term goal with some longer-term goals:

* Moving the Hactar firmware to Rust (especially on the UI chip)
* Having a Hactar software architecture that can be mostly developed/tested on
  normal desktop/mobile computers
* Having a high-fidelity Hactar emulator that runs on normal computers

An approach that seems promising for this is to use the [Tauri] framework to
build the normal-computer versions of the app.  Tauri is essentially a framework
for taking a Rust-based cross-platform app and giving it a WebView-based GUI.
As of version 2.0 (late 2024), Tauri can build such apps for all the major
desktop and mobile platforms.

So we could aim toward a long-range design where all of the Hactar application
logic is platform-neutral, and the platform specifics get provided by Tauri in
the normal-computer case and by custom code in the Hactar-device case.

```
     Hactar-Normal         Hactar-Device
         ^   ^                 ^   ^
         |   |                 |   |
         |   +--------+--------+   |
         |            |            |
         |    Application Logic    |
         |                         |
         |                         |
Tauri-based platform     Hactar device platform
```

We can work toward this by building a Tauri PTT app to start with, and then
refactoring the Tauri app and the device firmware toward more commonality.

## Tauri-based PTT

For PTT, the main things the platform needs to provide are:

1. Start-transmit and Stop-transmit signals
2. Audio input/output

Start/stop signals are easy to do from HTML, just using standard Tauri wiring.
For audio I/O, the [`cpal`] Rust library ([docs]) provides an audio platform that
works on all of Tauri's supported platforms. At least to first order -- some
configurations / parameters aren't supported on all platforms.  So we'll need a
bit of care and testing, but it seems like we should be able to have an app that
uses `cpal` for audio and Tauri for GUI.

```
             +-------+    +-------------+ 
Inputs ------|       |--->|             |
             |       |    |             |
             | tauri |    |             |
             |       |    |             |
Display <----|       |----|             |
             +-------+    |             |
                          | Application |
             +-------+    |             |
Microphone --|       |--->|             |
             |       |    |             |
             | cpal  |    |             |
             |       |    |             |
Speaker <----|       |----|             |
             +-------+    +-------------+
```

I have actually already prototyped a `cpal`-based [PTT-ish application] in this
repo.  It uses a different UI framework, but should show the basics of how to
plumb audio through a simple Rust app.

The main remaining question here is how to do MOQ.  We could pull in a Rust
library, but I don't think there's an interoperable one.  If we have working
C/C++ code, it's probably most expedient to just link the Rust code against it,
possibly using some `build.rs` glue.


[Tauri]: https://tauri.app/
[cpal]: https://github.com/rustaudio/cpal
[docs]: https://docs.rs/cpal/latest/cpal/
[PTT-ish application]: https://github.com/Quicr/hactar/tree/main/exploration/macos-audio
