# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Hactar is a hardware test device with three interconnected microcontrollers:
- **Management (mgmt)** - STM32F072: Handles USB communication, firmware uploads to other chips, and debug routing
- **User Interface (ui)** - STM32F405: Main processing, display, keyboard, audio
- **Network (net)** - ESP32S3: MOQ client using quicr library for network communication

The management chip acts as a gateway - it receives commands via USB and can put the board into different modes (ui_upload, net_upload, debug, etc.).

## Link Submodule (Hybridization)

The `link/` submodule contains the [Link repository](https://github.com/link-rs/link) which provides:
- **CTL** (`link/ctl/`) - Rust-based control program for flashing and device management
- **MGMT** (`link/mgmt/`) - Rust-based management firmware (replaces `firmware/mgmt/`)

The hybridization strategy uses:
- **From Link**: CTL control software and MGMT firmware
- **From Hactar**: UI firmware (`firmware/ui/`) and NET firmware (`firmware/net/`)

Build Link components with `cargo build` in their respective directories.

## Build Commands

Each firmware has its own directory under `firmware/` with a Makefile:

```bash
# Management chip (STM32F072) - plain Makefile
cd firmware/mgmt
make compile                    # Build
make upload port=/dev/ttyUSBx   # First flash (manual bootloader: BOOT+RESET)
make upload                     # Subsequent flashes (auto-detected)

# UI chip (STM32F405) - CMake-based
cd firmware/ui
make compile                    # Build (cmake -B build && cmake --build build)
make upload port=/dev/ttyUSBx   # Upload via serial (requires programmed mgmt chip)
make upload_debugger            # Upload via ST-Link

# Network chip (ESP32S3) - ESP-IDF
cd firmware/net
source $IDF_PATH/export.sh      # Required: ESP-IDF v5.4.2 must be on PATH
make compile                    # Build (idf.py build)
make upload port=/dev/ttyUSBx   # Upload via serial (~4 min for large binary)
```

Docker builds available for all chips: `make docker mft=compile`

## Code Formatting

Uses clang-format v18 with custom LLVM-based style. CI enforces formatting on PRs.

```bash
make format                     # Run in any firmware directory
```

Excluded from formatting: `lib/`, `Drivers/`, `Core/`, `net/include/sys`, `net/include/netinet`, `cmox`

## Architecture

### Inter-chip Communication
- `firmware/shared_inc/` - Shared headers used by multiple chips (serial protocols, ring buffers, link packet definitions)
- `ui_net_link.hh` - Protocol between UI and Network chips
- `ui_mgmt_link.h` / `net_mgmt_link.h` - Protocol definitions for mgmt communication
- `serial_packet.hh` / `serial_packet_manager.hh` - Packet framing over UART

### Firmware Structure
- `firmware/mgmt/src/` - C code: uart_router, chip_control, command_handler
- `firmware/ui/src/` - C++ code: app_main, screen, keyboard, audio, serial
- `firmware/net/core/` - C++ code: ESP-IDF components with quicr integration

### Python Tools
- `software/hactar-cli/` - Unified CLI for flashing and monitoring
  - `main.py flash --chip=<mgmt|ui|net> --port=/dev/ttyUSBx -bin=<path>`
  - `main.py monitor --port=/dev/ttyUSBx`

## Management Chip Commands

Send via serial monitor (`make monitor`):
- `reset` - Reset ui then net chip
- `ui_upload` - Enter ui chip upload mode
- `net_upload` - Enter net chip upload mode
- `debug` - Debug mode: forward serial from both chips to USB
- `debug_ui` / `debug_net` - Debug single chip

## Toolchain Requirements

- **STM32 (ui)**: arm-none-eabi-gcc, make, python3 with pyserial
- **ESP32 (net)**: ESP-IDF v5.4.2, make
- **Link CTL/MGMT**: Rust toolchain (cargo)
- **Flashing**: Link CTL program (primary), or st-flash, STM32_Programmer_CLI, openocd (optional)
