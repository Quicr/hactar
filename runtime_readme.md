# Installing the firmware

- USB-C Cable 
- [Rust](https://www.rust-lang.org/tools/install) with `thumbv6m-none-eabi` target and `default-stable`
- arm-none-eabi toolchain (for UI chip)
- ESP-IDF v5.4 (for NET chip). idf.py must be on your path.
- make
- Google Chrome or Firefox 151.0

Install Rust target for MGMT:
```bash
rustup default stable
rustup target add thumbv6m-none-eabi
cargo install cargo-binutils
rustup component add llvm-tools
```

#### Flash All Chips

The simplest way to set up a Hactar device is to flash all chips at once from the repository root:

```bash
cd hactar

# Build everything
make all

# Flash all chips (MGMT, UI, NET)
make flash
```

- NOTE- sometimes flashing fails to get the ui to respond because something is being too chatty on the serial lines AKA the bootup sequence from net. 
- This will be fixed in the future versions of the rust ctl/mgmt

#### Flash Individual Chips

*Management Chip*

```bash
make flash-mgmt
```

The MGMT chip must be flashed first as it handles flashing the other chips.

*UI Chip*

```bash
make flash-ui
```

*Network Chip*

```bash
make flash-net
```


# Modifying the analog power selection 

- Cut the pad between the center and the label 3.3V. 
- Solder a blob between VSYS and the center pad.

[![EV17 full view](img/ev17_fullview.png)]

[![EV17 zoomed in on solder pad](img/ev17_solderpad.png)]

# Configuring your Hactar

After flashing your hactar, you will need to tell the mgmt what version you are using (this will be deprecated hopefully sometime in the future)

```
cd hactar 
make set-version rev=17
make set-ssid ssid=my_ssid pwd=my_pwd
```

Next you will need to set the configurations by going to the [fl-identity](https://frontline.m10x.org/device) web console, NOTE- there is no way to get to this url from the admin console.
- Channel: Electronics
- Language: English (US)
- Click configure device 
    - A pop up to pick a tty device will show, select your hactar, should be /dev/ttyUSBx or /dev/cu.usbserial-x 
- Wait for it to configure and then close your window.

# Printable files

- Files can be found in models/ev17
- Print with Standard print settings on bambu labs.
- Lay all flat with the auto orientate
- Note- sometimes the hole on the power switch will sometimes get filled in if you use too large of layer thickness.
