# Hactar emulator

A native C++20, SDL2-based emulator for Linux and macOS. This first milestone runs
the **UI display**: the actual firmware splash/chat/menu views, screen layout,
fonts, icons, text editor, scrolling, rasterizer, dirty tiles, and double-buffered
RGB565 scanline pipeline.

## Build and run

Requirements: CMake 3.22+, a C++20 compiler (GCC 11+/recent Apple Clang), and SDL2
2.0.18+ development files. No ARM toolchain is needed for the emulator.

Linux (Debian/Ubuntu):

```sh
sudo apt install build-essential cmake libsdl2-dev
```

macOS (with Xcode Command Line Tools and Homebrew):

```sh
brew install cmake sdl2
```

From the repository root:

```sh
cmake -S emulator -B emulator/build
cmake --build emulator/build --parallel
./emulator/build/hactar-ui
```

CMake can also download and build a pinned SDL2 release:

```sh
cmake -S emulator -B emulator/build -DHACTAR_FETCH_SDL2=ON
cmake --build emulator/build --parallel
```

The fetch option needs internet access on the initial configure. An installed
SDL2 is preferred when available. If Homebrew SDL2 is not discovered, pass
`-DCMAKE_PREFIX_PATH="$(brew --prefix sdl2)"` to CMake.

## Display and controls

The logical display is the board's 320×240 landscape screen, initially shown at
3× scale. Resize the window to scale the display with its aspect ratio preserved.
The default launch shows the real splash bitmap for one second, then opens Chat.

| Control | Action |
| --- | --- |
| F1 | Splash view |
| F2 | Chat view |
| F3 | Main menu view |
| F5 | Force a full display refresh |
| F8 | Toggle the display backlight |
| Printable ASCII | Type into Chat's input line |
| Backspace | Edit input |
| Enter | Commit input as a local chat line |
| Escape / close window | Quit |

On macOS, function keys may require holding Fn. Input uses SDL text events and
the host keyboard layout. Characters outside the firmware's printable ASCII
font are ignored. Enter currently exercises local display scrolling; it does
not send a packet. Status icons start offline, with a simulated full battery.
Switching views clears the input draft.
The current firmware's Rooms and Wi-Fi views are empty, so they are not exposed.

Use `--view startup`, `--view chat`, or `--view menu` to select a view immediately
and disable the automatic startup transition. `--scale N` accepts 1 through 8.

## Headless verification and screenshots

```sh
ctest --test-dir emulator/build --output-on-failure
./emulator/build/hactar-ui --headless --view chat --frames 2 --screenshot /tmp/hactar-chat.bmp
./emulator/build/hactar-ui --headless --view startup --frames 2 --screenshot /tmp/hactar-splash.bmp
```

`--headless` selects SDL's dummy video driver, so these work without a desktop.
`--frames N` exits after N event/render iterations; it does not accelerate the
clock. Screenshots are native-resolution BMP files and reflect backlight/sleep
state. `--screenshot PATH` can also capture an interactive session on exit.

The integration checks exercise the real firmware views, RGB565 endianness,
partial-region transfers, input limits, history wraparound, scrolling, edge
pixels, dirty/full-refresh equivalence, and cursor timing. The same SDL backend
is used for the windowed application and headless checks. Screenshot round trips
also verify native resolution and backlight/sleep behavior. GitHub Actions builds
and runs these checks on both Linux and macOS.

## Hardware abstraction and code reuse

`firmware/ui/inc/graphics/display_driver.hh` is the shared display boundary.
`Screen` accepts a `DisplayDriver&`. The board's `ILI9341` and the emulator's
`SdlDisplayDriver` implement it. Pixel transfers contain RGB565 bytes in display
wire order (most significant byte first); no STM32 status types, registers, or
HAL headers cross this boundary.

The emulator build compiles these existing files directly:

- `firmware/ui/src/renderer.cc`
- `firmware/ui/src/screen.cc`
- `firmware/ui/src/battery.cc`
- `firmware/ui/src/graphics/graphics.cc`
- `firmware/ui/src/graphics/tile_map.cc`
- The original screen buffers, fonts, and icons from the firmware headers.

On the board, DMA half/full-complete interrupts advance the scanline pipeline.
On the host, `emulator::Ui::Tick()` pumps `Screen::Render()` and
`Screen::Transfer()` sequentially after synchronous pixel writes. Presentation,
SDL events, and timing stay on the main thread, as required by macOS. The native
entry point composes these shared components instead of running STM32 startup,
GPIO scanning, or the hardware-specific `app_main()`.

The emulator is a separate CMake project; the existing UI cross-build still uses
the board driver. The firmware logger also supports the host console and keeps
its existing `SetLogSender` hook for the future fake MGMT endpoint.

## Next milestones

1. **Audio:** extract the reusable audio-device operations into an interface,
   retain the board codec implementation, and implement host microphone capture
   and speaker playback using SDL. Reuse firmware processing and buffering.
2. **UART and simulated network:** connect the UI to fake MGMT (logs) and NET
   (network traffic) peers over OS byte streams. Reuse the repository's current
   UART packet framing and protocol definitions, with independently paced
   full-duplex directions and configurable baud/framing limits. Use replaceable
   endpoints so later UI, MGMT, and NET emulator processes can communicate over
   the same transport. Unix-domain sockets/pseudo-terminals provide OS-level
   byte transport between the emulator processes.

Audio devices and UART peers are the following stages, after this screen
milestone. Current logs go to stderr and the application starts without opening
an audio device or a network connection.
