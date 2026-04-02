# HACTAR

Hardware design for test device

## Table of contents:
1. [License](#license)
2. [Quick Start](#quick-start)
3. [Where To Find Things](#where-to-find-things)
4. [LED Definitions](#led-definitions)
5. [Hardware Technologies](#hardware-technologies)
6. [Prerequisites](#prerequisites)
    1. [Debian](#debian-prerequisites)
    2. [MacOS](#macos-prerequisites)
    3. [Windows](#windows-prerequisites)
7. [Firmware](#firmware)
    1. [Management](#management)
    2. [User Interface](#user-interface)
    3. [Network](#network)
    4. [STM32 Toolchain](#stm32-toolchain-installation)
    5. [ESP32 Toolchain](#esp32-toolchain-installation)
8. [Software](#software)
    1. [Hactar-cli](#hactar-cli)
9. [Hactar Setup](#hactar_setup)
10. [Troubleshooting](#troubleshooting)

## License

The license for information in this repository is in [LICENSE](https://github.com/Quicr/hactar/blob/main/LICENSE).  This license covers everything in the repository except for the directory `firmware/ui/dependencies/cmox`, which is covered by the license in [firmware/ui/dependencies/cmox/LICENSE].

## Quick Start

The easiest way to build and flash all firmware is from the repository root:

```bash
# Build everything (requires Rust and ESP-IDF)
make all

# Flash all chips (requires device connected via USB)
make flash

# Or flash individual chips
make flash-mgmt
make flash-ui
make flash-net
```

For new developers looking to start developing on Hactar quickly there are quick start python scripts located in etc/quick-start, that will automatically download and setup your dev environment.

## Where To Find Things

- etc - Extra files

- firmware - Code for UI, Net, and Mgmt chips

- hardware - Schematics and PCB designs

- models - 3D models for project

- software - Tools and Utilities

## LED definitions

LED 1 - Power and Network status:
- Solid Blue - Everything is OK (Connected to MoQ + WiFi + Relay + AI)
- Pulsing Blue - Connecting / Searching (WiFi or MoQ)
- Pulsing Orange - WiFi OK but not connected MoQ
- Blinking Orange - Network Error (bad credentials or no WiFI in range)
- Solid Yellow - Low battery (<20%) - overrides blue states.
- Pulsing Yellow - Charging
- Solid Red - Unprogrammed
- Blinking Red - Hardware/Firmware Failure
- Blinking Purple - AI not connected/not working when user attempts to use it

LED 2 - User Activity & Notifications:
- Off - No messages / actions 
- Blinking Blue - Unread messages/notifications waiting
- Blinking Orange - User error (no headset detected)
- Solid Blue - User holding a PTT/PTTAI button.

## Hardware Technologies
- [KiCad 9](https://www.kicad.org/)
- [FreeCAD v1.0](https://www.freecad.org/)

## Prerequistites

### Debian Prerequistites

#### Groups
``` usermod $USER -aG dialout -aG plugdev ```

#### Rules.d

``` cp etc/rules.d/* /etc/udev/rules.d/ ```

### MacOS Prerequistites

[Homebrew](https://brew.sh/) is the recommended for installing everything in this project.


### Windows Prerequistites

We only support using [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install) for development, you are free to use native windows, but it is untested on actually getting everything in this codebase running.

Any and all instructions for Debian will work on WSL and is recommended you follow those.

## Firmware

### Firmware Technologies
- [Rust](https://www.rust-lang.org/tools/install) (for MGMT chip and CTL tool)
- [GNU arm-none-eabi](https://developer.arm.com/downloads/-/gnu-rm) (for UI chip)
- [ESP-IDF v5.4.2](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/get-started/index.html) (for NET chip)
- [STM32 CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)

The Firmware is split into 3 categories. Management, User Interface, and Network.
- Management - STM32F072 (Rust, in `link/mgmt`)
- User Interface - STM32F405 (C++, in `firmware/ui`)
- Network - ESP32S3 (C++, in `firmware/net`)

### CTL Tool

The `ctl` tool (in `link/ctl`) is a Rust command-line program that handles flashing and device communication. It auto-detects connected Hactar devices and provides commands for:

- Flashing firmware to all chips
- Sending commands to chips (ping, version, config)
- Monitoring device logs

Build and run the CTL tool:
```bash
# From repository root
make run-ctl

# Or directly
cd link/ctl && cargo run --release
```

### [___Management___](#management)

The management chip (STM32F072) is responsible for flashing firmware to the UI and NET chips, and routing communication between the USB interface and the other chips.

The MGMT firmware is written in Rust and located in `link/mgmt`.

##### To view your device list

- ##### Debian/MacOS

    `ls /dev/ttyUSB*` or `ls /dev/cu.usbserial-*`

- ##### Windows

    `Open printers and devices`

    `Find serial com ports`

    [Attach to WSL](https://learn.microsoft.com/en-us/windows/wsl/connect-usb)

    `In WSL check dmesg to find which serial port your device binded to`

*Flashing*

From the repository root:

```bash
make flash-mgmt
```

Or using the CTL tool directly:

```bash
link/ctl/target/release/ctl mgmt flash link/mgmt/target/thumbv6m-none-eabi/release/mgmt.bin
```

#### Commands

Run the CTL tool to interact with the device:

```bash
make run-ctl
```

Or use individual commands directly:

```bash
# Test connectivity
link/ctl/target/release/ctl hello

# Ping that traverses all chips (MGMT -> UI -> NET -> MGMT)
link/ctl/target/release/ctl circular-ping

# MGMT commands
link/ctl/target/release/ctl mgmt ping
link/ctl/target/release/ctl mgmt version

# UI commands
link/ctl/target/release/ctl ui ping
link/ctl/target/release/ctl ui version

# NET commands
link/ctl/target/release/ctl net ping
link/ctl/target/release/ctl net version
```

Legacy commands (via hactar-cli monitor):

- **hard reset** - Restarts the main function of the management chip
- **reset** - Resets `ui` chip and then `net` chip
- **reset ui** - Resets `ui` chip
- **reset net** - Resets `net` chip
- **stop ui** - Hold `ui` chip in reset
- **stop net** - Hold `net` chip in reset
- **enable logs** - Enables logs on the management side
- **disable logs** - Disable logs on the management side

#### UI commands

- **ui version** - Get the ui version
- **ui clear_config** - Clear the eeprom config on the ui
- **ui set_sframe** - Sets the sframe key
- **ui get_sframe** - Gets the sframe key
- **ui toggle_logs** - Toggles logging on the ui side
- **ui disable_logs** - Disables logging on the ui side
- **ui enable_logs** - Enables logging on the ui side

#### NET commands

- **net version** - Get the net version
- **net clear_storage** - Clears the storage to default values
- **net set_ssid** - Sets the wifi ssid name and password and saves it in the NVS
- **net get_ssid_names** - Gets all of the ssid names stored
- **net get_ssid_passwords** - Gets all of the ssid passwords stored
- **net clear_ssids** - Clears all ssids from storage
- **net set_moq_url** - Sets the moq url
- **net get_moq_url** - Gets the current moq url
- **net toggle_logs** - Toggles logs on the net side
- **net disable_logs** - Disables logs on the net side
- **net enable_logs** - Enables logs on the net side
- **net disable_loopback** - Disables looping outputted data back in
- **net enable_loopback** - Enables looping outputted data back in
- **net set_fl_config** - Sets the frontline config
- **net burn_efuse** - Burns the efuse to enable jtag debugging

#### Root Makefile Targets

From the repository root:

- `make all` - Build all firmware (MGMT, UI, NET) and the CTL tool
- `make flash` - Flash all chips
- `make flash-mgmt` - Flash only the MGMT chip
- `make flash-ui` - Flash only the UI chip
- `make flash-net` - Flash only the NET chip
- `make run-ctl` - Run the CTL tool interactively
- `make clean` - Clean all build artifacts

**Source code**

The Management Chip firmware is written in Rust and located in `link/mgmt`.

### User Interface

The User Interface chip is where most of the processing takes place. It utilizes a STM32F405 chip empowered by the STM HAL Library. To generate the baseline HAL Library code and Makefile, ST's CubeMX was used.

**Makefile Target Overview**

- `all` - Builds the target bin, elf, and binary files and uploads the code to the User Interface chip. Output firmware/build/ui

- `compile` - Builds the target bin, elf, and binary. Output firmware/build/ui

- `upload` - Uploads the build target to the User Interface chip using the our custom flasher tool with serial usb-c uploading. See [Python Flasher](#py_flasher)

- `upload_debugger` - Uploads the build target to the User Interface chip using the st-flash tools. Please see
[st-flash installation](#stflash_installation) for instructions on installing.

- `upload_cube_uart` - Uploads the build target to the User Interface chip using the STM32_Programmer_CLI with serial usb-c uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `upload_cube_swd` - Uploads the build target to the User Interface chip using the STM32_Programmer_CLI with st-link using SWD uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `docker` - Uses docker to compile, flash, and monitor the ui chip. By default it will compile the code. To change the functionality, pass a target of the Makefile into the parameter `mft` when being invoked.
    - Ex. `make docker mft=upload`
    - Ex. `make docker mft=monitor`

- `monitor` - Opens a serial communication line with the management chip with a python script that allows for sending of commands.

- `openocd` - Opens a openocd debugging instance. See [Installation](#stm_installation) for instructions on installing openocd.

- `docker-clean` - Removes dockere image that was generated when running docker target.

- `clean` - Deletes the firmware/build/ui directory.

**Source code**

All source code for the User Interface can be found in firmware/ui, firmware/shared, and firmware/shared_inc. Adding C/C++ files in firmware/ui/src and firmware/ui/inc does not require any changes in the makefile.

Making a shared header between chips should be added into firmware/shared_inc.

### **Network**

The network chip is a MOQ client using [quicr](https://github.com/Quicr/libquicr). The network chip communicates with the UI chip via UART serial communication.

We are leveraging the **esp-idf** framework.

**Makefile Target Overview**

- `all` - Runs the `compile` and `upload` targets.

- `compile` - Compiles the net code using the command `idf.py compile`

- `upload` - Uploads the build target to the Network chip using the our custom flasher tool with serial usb-c uploading. See [Python Flasher](#py_flasher)

- `upload_debugger` - Uploads the net code using openocd over an ESP-PROG

- `upload_esptool` - Uploads the network chip code by first sending a command to the mgmt chip to put the board into **net upload** mode. Then uses `esptool.py upload`

- `docker` - Uses docker to compile, flash, and monitor the ui chip. By default it will compile the code. To change the functionality, pass a target of the Makefile into the parameter `mft` when being invoked.
    - Ex. `make docker mft=upload`
    - Ex. `make docker mft=monitor`

- `monitor` - Opens a serial communication line with the management chip with a python script that allows for sending of commands.

- `clean` - Deletes the firmware/net/build directory.

- `fullclean` - Invokes idf.py fullclean.

- `docker-clean` - Removes dockere image that was generated when running docker target.

<b>Source code</b>

All source code for the network chip can be found in `firmware/net`, `firmware/shared`, and `firmware/shared_inc`.

## MGMT Toolchain Installation

The following tools are required for building the Management chip firmware and CTL tool.
- [Make](#make) \[required]
- [Rust](#rust) \[required]

##### **Make**

Make is used for compiling and uploading automation for each chip firmware.

*Debian*

```bash
sudo apt install make
```
*MacOS*

```brew
brew install make
```

##### Rust

Rust is required for building the MGMT firmware and CTL tool.

*All Platforms*

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

After installing Rust, add the ARM Cortex-M0 target for MGMT:

```bash
rustup target add thumbv6m-none-eabi
cargo install cargo-binutils
rustup component add llvm-tools
```

## UI Toolchain Installation

The following tools are used for building the User Interface chip firmware.
- [ARM None Eabi Toolchain](#arm-none-eabi-toolchain) \[required]
- [Python3](https://www.python.org/downloads/) \[optional]
- [ST-Flash](#st-flash) \[optional]
- [STM32_Programmer_CLI](#stm32-programmer-cli) \[optional]
- [OpenOCD](#openocd) \[optional]

##### ARM None Eabi Toolchain

*Debian*

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi
```

*MacOS*

```sh
brew install --cask gcc-arm-embedded
```

##### ST Flash

*Debian*

```bash
sudo apt install stlink-tools
```

*MacOS*

```bash
brew install stm32flash
```

##### STM32 Programmer CLI

The STM32 Programmer CLI comes packages with the STM32 Cube Programmer. You'll need to download and install The STM32 Cube Programmer and add the STM32_Programmer_CLI binary to your path.
```
https://www.st.com/en/development-tools/stm32cubeprog.html
```

*Debian*

- Open your `.bashrc` file:
    ```
    sudo nano ~/.bashrc
    ```

- Add the following line to the `.bashrc` file
    ```bash
    export PATH="$PATH:/path/to/stm_cube_programmer/bin"
    ```

*MacOS*

- TODO

#### OpenOCD

Open On Chip Debugger, created by Dominic Rath, is a debugging software that can be used with micro-controllers to run debugging tools and stepping through code.

*Debian*

Run the following command:

```bash
sudo apt install openocd gdb-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

If you don't have python3.8 installed, which is used for arm-none-eabi-gdb, then you'll need to install it
```bash
sudo add-apt-repository ppa:deadsnakes/ppa
apt update
sudo apt install python3.8
```

*MacOS*

Run the following commands in a terminal to install `openocd`

```brew
brew update
```

```brew
brew install open-ocd --HEAD
```
## ESP32 Toolchain Installation

This project leverages the usage of the ESP-IDF Library for the network chip.

The following tools are used for the Network.
- [Make](#make) \[required]
- [esp-idf v5.4.2](#esp-idf) \[required]

##### ESP IDF

- [Debian](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/get-started/linux-macos-setup.html)
- [MacOS](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/get-started/linux-macos-setup.html)

## Software 

### Hactar-cli

Hactar-cli is a multi-faceted tool, where it can flash, monitor, and send commands. We ship a version of python that includes pyserial with it.

#### Flasher

Generally the flasher is used automatically in the Makefile. However, you can flash whatever binary you want onto a chip that the flasher is designed for by running it as python script in the firmware/flasher folder.

ex.

```sh
python3 main.py flash --port=<port> --baud=<baudrate> --chip="<chip>" -bin="./build/app.bin"
```

You can omit passing a port and the flasher will attempt to find a Hactar board by searching your active usb serial ports.

```sh
python3 main.py flash --baud=115200 --chip="ui" -bin="./build/ui.bin"
```

#### Monitor

The serial monitor is a very simple serial monitor that allows a user to send commands to the mgmt chip as well as read serial logs.

This monitor can be used by the following command:

```sh
python main.py monitor --port=<port> --baud=<baudrate>
```

Ex.
```sh
python main.py monitor --port=/dev/ttyUSB0
```

### Hactar Setup

#### Prerequisites

- USB-C Cable
- [Rust](https://www.rust-lang.org/tools/install) with `thumbv6m-none-eabi` target
- arm-none-eabi toolchain (for UI chip)
- ESP-IDF v5.4 (for NET chip)
- make

Install Rust target for MGMT:
```bash
rustup target add thumbv6m-none-eabi
cargo install cargo-binutils
rustup component add llvm-tools
```

#### Flash All Chips

The simplest way to set up a Hactar device is to flash all chips at once from the repository root:

```bash
# Build everything
make all

# Flash all chips (MGMT, UI, NET)
make flash
```

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

Note: The NET chip firmware is large and takes about 4 minutes to upload.

*Debug mode example*

- Prerequisites
    - Python3
        - **pyserial** - download using `pip3 install pyserial`
    - make
    - USB-C Cable
- Have your hactar board programmed and plugged in using a USB-C cable
- Navigate to either `hactar/firmware/ui` or `hactar/firmware/net`
    - **NOTE** - Update your ports appropriately in the makefile
- Enter `make py_monitor` into your terminal
- You should see a serial monitor open in your terminal requesting a command. Enter `debug` to put the hactar board into debugging mode.
    - See [Management Commands](#management_commands) for more commands that can be sent to the hactar board.
- The first and third LED from the left will turn blue indicating debug mode and your console will receive serial debug messages from the UI and Net chips.
- Enter `exit` to leave the monitor

## Troubleshooting

### USB device disconnects shortly after connecting randomly

- CH340 chips are shared with a program called brltty which is for braille devices. Debian automatically will try to connect it to brltty and therefore is in use, you can unplug and replug until it doesn't take control of the device or you can uninstall brltty if you don't need braille device.
    - ``` sudo apt remove brltty ```
