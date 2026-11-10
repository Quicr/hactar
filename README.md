# Device updater

Static GitHub Pages updater written in plain JavaScript, using Web Serial. No
build step, WASM, CDN dependencies, or backend is needed.

## Usage

Open the site in desktop Chrome or Edge, connect the device over USB, select the
chips, and click **Update Device**. Select the device's USB serial port. Close
`ctl`, serial monitors, and other programs using that port first.

The default update order is **MGMT → UI → NET**. Individual chips can also be
selected. UI and NET updates require MGMT firmware implementing the LINK protocol
from `~/code/link`. The update log is available under **Advanced Options**.

MGMT automatically enters its ROM bootloader using the EV16 DTR/RTS reset
sequence. For boards without automatic reset, set MGMT BOOT0 high, reset the
device, and enable **MGMT is already in bootloader mode** before starting.
After a manual update, release BOOT0 and reset the device to boot normally.

## Firmware files

Publish these files alongside `index.html`:

| Chip | File | Flash address |
| --- | --- | --- |
| MGMT (STM32F072CB) | `build/mgmt/mgmt.bin` | `0x08000000` |
| UI (STM32F405RG) | `build/ui/ui.bin` | `0x08000000` |
| NET (ESP32-S3) | `build/net/bootloader.bin` | `0x0000` |
| NET | `build/net/partition-table.bin` | `0x8000` |
| NET | `build/net/net.bin` | `0x10000` |

NET addresses follow the existing ESP-IDF build's `flasher_args.json`. Its app
must occupy the partition starting at `0x10000`; if the firmware layout changes,
update the addresses in `scripts/flasher.js` together with the binaries.
All selected images download before flashing starts. Only the required STM32
pages/sectors and NET image regions are erased. STM32 images are verified by
reading them back; NET images are checked using the ROM's MD5 command.

## Protocol

- MGMT ROM: 115200 baud, 8E1, STM32 AN3155 bootloader protocol.
- Running MGMT: 1000000 baud, 8E1, `LINK` frames with little-endian type/length.
- UI ROM: 115200 baud, 8E1 on the internal UART, tunneled with `ToUi`/`FromUi`.
- NET ROM: internal UART remains at 1000000 baud, 8N1, SLIP packets tunneled
  with `ToNet`/`FromNet`. Uses the ROM directly without uploading a stub.
- Only Hello, SetPin, SetUiBaudRate, and the two data tunnels are implemented.
  Reset timings and pin identifiers follow `link/src/ctl/{core,flash}.rs`.

The other chip is held in reset during UI/NET flashing and restored afterward.

## Local development

Serve the repository root, for example:

```sh
python3 -m http.server 8000
```

Open `http://localhost:8000`. Web Serial requires HTTPS or localhost; opening
`index.html` directly is not supported. GitHub Pages supplies HTTPS.

Protocol checks use Node's built-in test runner:

```sh
node --experimental-default-type=module --test tests/*.test.js
```

Actual USB flashing must also be checked with a physical device.
