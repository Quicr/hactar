# HACTAR

Hardware design for test device

## Table of contents:
1. [Where To Find Things](#where)
2. [Hardware](#hardware)
3. [Firmware](#firmware)
    1. [Management](#management)
    2. [User Interface](#ui)
    3. [Network](#network)
    4. [Security](#security_layer)
4. [Tools](#tools)
    1. [Echo Server](#echo_server)
    2. [Python Serial Monitor](#serial_monitor)
5. [STM32 Toolchain](#stm_installation)
6. [ESP32 Toolchain](#esp_installation)
7. [Hactar Installation](#hactar_installation)
    1. [EV1](#ev1) - WIP
    2. [EV2](#ev2) - WIP
    3. [EV3](#ev3) - WIP
    4. [EV4](#ev4) - WIP
    5. [EV5](#ev5) - WIP
    6. [EV6](#ev6) - WIP
    7. [EV7](#ev7) - WIP
    8. [EV8](#ev8) - Current

<h2 id="where">Where To Find Things</h2>

- datasheets - all the datasheets for parts used

- docs - documents about this project

- firmware - the code for testing and supporting the hardware

- hardware - the schematics and PCB designs

- models - Contains 3D models for project

- photos - image from the project as it progresses

- productions - files used for manufacturing that are generated from the
stuff in hardware

<h2 id="hardware">Hardware</h2>

<h3 id="hw_technologies">Technologies</h3>

- KiCAD

<h2 id="firmware">Firmware</h2>

The Firmware is split into 4 categories. Management, User Interface, Security, and Network.
- Management - STM32F072
- User Interface - STM32F405
- Network - ESP32S3

<h3 id="management"><b>Management</b></h3>

The management chip is responsible for uploading firmware to the stm32 - main chip and the esp32 network chip using a ch340 usb chip.

The management chip receives commands from the usb communication that informs it what chip we are currently uploading.

<h4 id="management_commands">Commands</h4>

- **reset** - Resets `ui` chip and then `net` chip. Accepts commands.
- **ui_upload** - Puts the management chip into `ui` chip upload mode. (STM32f05)
- **net_upload** - Puts the management chip into `net` chip upload mode. (ESP32S3)
- **debug** - Puts the management chip into debugging mode where serial messages from both `ui` and network chips are transmitted back to the usb interface. This is uni-directional from the management chip to the computer. However, all commands work during this mode. In the future sending messages to the `ui` and `net` chip will be supported
- **debug_ui** (future feature) - Only reads serial messages from the `ui` chip.
- **debug_net** (future feature) - Only reads serial messages from the `net` chip.
- **debug_ui_only** (future feature) - Puts the `ui` chip into normal mode, holds the `net` chip in reset mode, and allows for serial communication to the `ui` chip.
- **debug_net_only** (future feature) - Puts the `net` chip into normal mode, holds the `ui` chip in reset mode, and allows for serial communication to the `net` chip.


<h3 id="ui"><b>User Interface</b></h3>

The User Interface chip is where most of the processing takes place. It utilizes a STM32F405 chip empowered by the STM HAL Library. To generate the baseline HAL Library code and Makefile, ST's CubeMX was used.

<b>Target Overview</b>

- `all` - Builds the target bin, elf, and binary files and uploads the code to the User Interface chip. Output firmware/build/ui

- `compile` - Builds the target bin, elf, and binary. Output firmware/build/ui

- `upload` - Uploads the build target to the User Interface chip using the STM32_Programmer_CLI with serial usb-c uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `upload_swd` - Uploads the build target to the User Interface chip using the STM32_Programmer_CLI with swd uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `program` - Runs the compile target, then the upload target.

- `monitor` - Opens a serial communication line with the management chip in debug mode using screen.

- `py_monitor` - Opens a serial communication line with the management chip with a python script that allows for sending of commands.

- `openocd` - Opens a openocd debugging instance. See [Installation](#stm_installation) for instructions on installing openocd.

- `info` - Displays the build path, include files, object files, dependencies and vpaths used by the compiler.

- `dirs` - Makes all directories required for by the Makefile to compile and upload.

- `clean` - Deletes the firmware/build/ui directory.

<b>Source code</b>

All source code for the User Interface can be found in firmware/ui and firmware/shared_inc. Adding C/C++ files in firmware/ui/src and firmware/ui/inc does not require any changes in the makefile.

Making a shared header between chips should be added into firmware/shared_inc.

At any point if you're including or using any additional STM32 HAL library files such as `stm32f4xx_hal_sd.h` then you must add the respective source file `stm32f4xx_hal_sd.c` to HAL C Sources line ~50 in firmware/ui/Makefile.

Additionally, any new Assembly source files need to be added to the ASM sources line ~77 in firmware/ui/Makefile.

<h3 id="network"><b>Network</b></h3>

The network chip is responsible for all communications between servers and the entire board. The network chip communicates with the UI chip via UART serial communication.

We are leveraging the **esp-idf** framework.

<b>Target Overview</b>

- `all` - Runs the `compile` and `upload` targets.

- `compile` - Compiles the net code using the command `idf.py compile`

- `upload` - Uploads the net code by first sending a command to the mgmt chip to put the board into **net upload** mode. Then uses `esptool.py upload`

- `program` - Runs the compile target, then the upload target.

- `monitor` - Opens a serial communication line with the management chip in debug mode using screen.

- `py_monitor` - Opens a serial communication line with the management chip with a python script that allows for sending of commands.

- `clean` - Deletes the firmware/net/build directory.

<b>Source code</b>

All source code for the network chip can be found in `firmware/net` and `firmware/shared_inc`.

<h3 id="security"><b>Security</b></h3>

WIP


<h2 id="tools">Tools</h2>

<h3 id="echo_server"><b>Echo Server</b></h3>
    Very basic server that echo's the message it receives.

<h3 id="serial_monitor"><b>Python Serial Monitor</b></h3>

Located in firmware/tools

Requirements
- py_serial - ```pip install py_serial```

This monitor can be used by the following command:
`python monitor.py \[port] \[baudrate]`

ex.

`python monitor.py /dev/ttyUSB0 115200`

<h2 id="stm_installation">Installation - STM32 Toolchain</h2>

The following tools are used for Management and User Interface.
- [Make](ui_make) \[required]
- [ARM GNU Toolchain](arm-gnu) \[required]
- [STM32_Programmer_CLI](stm_prog_cli) \[required]
- [OpenOCD](openocd) \[optional]

<b id="ui_make">Make \[required]</b>

Make is used for compiling and uploading automation for each chip firmware.

<i>Debian</i>

```bash
sudo apt install make
```

<i>Windows</i>

- Download make.exe from [Source Forge](https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81.exe/download?use_mirror=phoenixnap&download=)

- Save it to C:/bin or where-ever you want it. Then add the location to the your path.

- Open the start search, and type `env`. Select `"Edit the system environment variables"`

- Click `"Environmental Variables..."`

- Under `"System Variables"` find the `"Path"` entry, click `"Edit..."` button.

- Click the `"New"` button and enter `DRIVE:\path\to\make_location`
    - Note. Replace <i>DRIVE</i> with your appropriate drive letter, C, D, E... etc

- You may need to restart your system after changing your system path.

<i>MacOS</i>

```brew
brew install make
```

<b id="arm-gnu">ARM GNU Toolchain \[required]</b>

<i>Debian</i>

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi
```

<i>Windows</i>

- Download the appropriate ARM GNU Toolchain for your system https://developer.arm.com/downloads/-/gnu-rm

<i>MacOS</i>

- TODO

<br/>

<b id="stm_prog_cli">STM32 Programmer CLI \[required]</b>

The STM32 Programmer CLI comes packages with the STM32 Cube Programmer. You'll need to download and install The STM32 Cube Programmer and add the STM32_Programmer_CLI binary to your path.
```
https://www.st.com/en/development-tools/stm32cubeprog.html
```

<i>Debian</i>

- Open your `.bashrc` file:
    ```
    sudo nano ~/.bashrc
    ```

- Add the following line to the `.bashrc` file
    ```bash
    export PATH="$PATH:/path/to/stm_cube_programmer/bin"
    ```

- NOTE - If you are getting an error saying that libusb needs permission to write usb, then you'll need to do an additional step.
    - Navigate to
        ```bash
        /path/to/stm_cube_programmer/Drivers/rules
        ```
    - Edit 49-stlinkv2.rules
        - Add the following to the bottom of the file
            ```
            SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", SYSFS{idVendor}=="3748", \
                MODE="0666", \
                GROUP="plugdev"
            ```
    - Copy all of the rules to /etc/udev/rules.d/
        - `cp /path/to/stm_cube_programmer/Drivers/rules/*.* /etc/udev/rules.d/`
    - Unplug then plug in your device and STMCubeProgrammerCLI should work.


<i>Windows</i>

- Open the start search, and type `env`. Select `"Edit the system environment variables"`

- Click `"Environmental Variables..."`

- Under `"System Variables"` find the `"Path"` entry, click `"Edit..."` button.

- Click the `"New"` button and enter `DRIVE:\path\to\stm_cube_programmer\bin`
    - Note. Replace <i>DRIVE</i> with your appropriate drive letter, C, D, E... etc

- You may need to restart your system after changing your system path.

<i>MacOS</i>

- TODO


<br/>

<b id="openocd">OpenOCD \[optional]</b>

Open On Chip Debugger, created by Dominic Rath, is a debugging software that can be used with micro-controllers to run debugging tools and stepping through code.

<br/>

<i>Debian</i>

Run the following command:

```bash
sudo apt install openocd gdb-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

<i>Windows</i>

- Download the appropriate zip for your system https://openocd.org/pages/getting-openocd.html

- Open the start search, and type `env`. Select `"Edit the system environment variables"`

- Click `"Environmental Variables..."`

- Under `"System Variables"` find the `"Path"` entry, click `"Edit..."` button.

- Click the `"New"` button and enter `DRIVE:\path\to\openocd\bin`
    - Note. Replace <i>DRIVE</i> with your appropriate drive letter, C, D, E... etc

- You may need to restart your system after changing your system path.

<i>MacOS</i>

Run the following commands in a terminal to install `openocd`

```brew
brew update
```

```brew
brew install open-ocd --HEAD
```
<h2 id="esp_installation">Installation - ESP32 Toolchain</h2>

This project leverages the usage of the Arduino Library for the network chip.

The following tools are used for the Network.
- [Make](net_make) \[required]
- [esp-idf](esp_idf) \[required]

<b id="net_make">Make \[required]</b>

Make is used for compiling and uploading automation for each chip firmware.

<i>Debian</i>

```bash
sudo apt install make
```

<i>Windows</i>

- Download make.exe from [Source Forge](https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81.exe/download?use_mirror=phoenixnap&download=)

- Save it to C:/bin or where-ever you want it. Then add the location to the your path.

- Open the start search, and type `env`. Select `"Edit the system environment variables"`

- Click `"Environmental Variables..."`

- Under `"System Variables"` find the `"Path"` entry, click `"Edit..."` button.

- Click the `"New"` button and enter `DRIVE:\path\to\make_location`
    - Note. Replace <i>DRIVE</i> with your appropriate drive letter, C, D, E... etc

- You may need to restart your system after changing your system path.

<i>MacOS</i>

```brew
brew install make
```

<b id="esp_idf">esp-idf \[required]</b>

<i>Debian</i>

- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html

<i>Windows</i>

- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html


<i>MacOS</i>

- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html


<h2 id="hactar_installation">Hactar Installation</h2>

<h3 id="EV1">EV1</h2>

WIP

<h3 id="EV2">EV2</h2>

WIP

<h3 id="EV3">EV3</h2>

WIP

<h3 id="EV4">EV4</h2>

WIP

<h3 id="EV5">EV5</h2>

WIP

<h3 id="EV6">EV6</h2>

WIP

<h3 id="EV7">EV7</h2>

WIP

<h3 id="EV8">EV8 - Current</h2>

<i>Management Chip</i>

- Prerequisites
    - Stlink-v2
    - STM32 Cube Programmer CLI
    - arm-none-eabi-g++
    - make
- Hook up the stlink-v2 to the connector beside the usb-c connector.
    - Note, you will probably want to make a connector that has female dupoint headers on one end and a PH 2.0 connector on the other end.
- Build the mgmt code by navigating to `hactar/firmware/mgmt` and entering `make compile`
- Upload the mgmt code by entering `make upload`
- After this you should see a couple of LED's light up

<i>Userinterface Chip</i>

- Prerequisites
    - A programmed `management chip`
    - USB-C Cable
    - STM32 Cube Programmer CLI
    - arm-none-eabi-g++
    - make
- Plug in the USB-C cable to the Hactar board.
- Build the ui code by navigating to the `hactar/firmware/ui` folder and entering `make compile`
- Upload the ui code by entering `make upload`
    - **NOTE** - Update your the `port` variable, based on your OS and usb input, in the `hactar/firmware/ui/makefile`
    - Once the process begins, a python script is called to send the command `ui_upload` to the management chip, turns off other LED's and turns on the third LED from the left, and puts the ui chip into bootloader mode.
    - Then the stm32 cube programmer cli is called to upload the firmware.
- After finishing uploading the firmware to the ui chip, it will return to running mode after 5 seconds.

<i>Network Chip</i>

- Prerequisites
    - A programmed `management chip`
    - USB-C Cable
    - esp-idf - must be on your path
    - make
- Plug in the USB-C cable to the Hactar board.
- Build the net code by navigating to the `hactar/firmware/net` folder and entering `make compile`
- Upload the net code by entering `make upload`
    - **NOTE** - Update your the `port` variable, based on your OS and usb input, in the `hactar/firmware/net/makefile`
    - Once the process begins, a python script is called to send the command `net_upload` to the management chip, turns off other LED's and turns on the first LED from the left, and puts the net chip into bootloader mode.
    - Then the stm32 cube programmer cli is called to upload the firmware.
- After finishing uploading the firmware to the net chip, the management chip will return to running mode.

<i>Debug mode example</i>

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