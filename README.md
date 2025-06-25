# HACTAR

Hardware design for test device

## Table of contents:
1. [License](#License)
2. [Quick-Start](#quick_start)
3. [Where To Find Things](#where)
4. [Hardware](#hardware)
5. [Prerequisites](#prereq)
    1. [Linux](#linux-prereq)
    2. [MacOS](#macos-prereq)
    3. [Windows](#windows-prereq)
6. [Firmware](#firmware)
    1. [Management](#management)
    2. [User Interface](#ui)
    3. [Network](#network)
    4. [Security](#security_layer)
    5. [STM32 Toolchain](#stm_installation)
    6. [ESP32 Toolchain](#esp_installation)
7. [Tools](#tools)
    1. [Python Flasher](#py_flasher)
    2. [Python Serial Monitor](#serial_monitor)
8. [Hactar Setup](#hactar_setup)
9. [Troubleshooting]

## License

The license for information in this repository is in [LICENSE].  This license covers everything in the repository except for the directory `firmware/ui/dependencies/cmox`, which is covered by the license in [firmware/ui/dependencies/cmox/LICENSE].

## <h2 id="quick-start">Quick Start</h2>
For new developers looking to start developing on Hactar quickly there are quick start python scripts located in etc/quick-setup, that will automatically download and setup your dev environment.

## <h2 id="where">Where To Find Things</h2>

- datasheets - all the datasheets for parts used

- docs - documents about this project

- firmware - the code for testing and supporting the hardware

- hardware - the schematics and PCB designs

- models - Contains 3D models for project

- photos - image from the project as it progresses

- productions - files used for manufacturing that are generated from the
stuff in hardware

## <h2 id="hardware">Hardware</h2>

### <h3 id="technologies">Technologies</h3>

- [KiCAD9](https://www.kicad.org/)
- [FreeCAD](https://www.freecad.org/)
- [arm-none-eabi](https://developer.arm.com/downloads/-/gnu-rm)
- [ESP-IDF](https://idf.espressif.com/)
- [STM32 CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)


## <h2 id="prereq">Pre-

### <h3 id="linux-prereq">Prerequistites</h3>

#### Groups
``` usermod $USER -aG dialout -aG plugdev ```

#### Rules.d

``` cp etc/rules.d/* /etc/udev/rules.d/ ```

<h2 id="firmware">Firmware</h2>

The Firmware is split into 3 categories. Management, User Interface, and Network.
- Management - STM32F072
- User Interface - STM32F405
- Network - ESP32S3

### <h3 id="management"><b>Management</b></h3>

The management chip is responsible for uploading firmware to the stm32 - main chip and the esp32 network chip using a ch340 usb chip.

The management chip receives commands from the usb communication that informs it what chip we are currently uploading. As well as debugging. It is able to receive log messages from the ui and net chips and push them out to the usb interface.

#### <h4 id="management_flashing">Flashing</h4>
On the first flash you'll need to figure out what device port your hactar is on. You can do this in several ways depending on which operating system you are on. Start by viewing your devices, and then plug in your hactar, and then view your devices again and see what was added to your device list.


<h5>To view your device list:</h5>

- <h5>Linux/MacOS</h5>

    `ls /dev/ttyUSB*`

- <h5>Windows</h5>

    `Open printers and devices`

    `Find serial com ports`

To flash the management chip you'll need to press the boot and reset buttons on the board in the following manner:

<i id="button_press_order">Button press order</i>
- Press and hold boot button (left).
- Press and release reset button (right).
- Release boot button.

<i>First flash</i>

- In hactar/firmware/mgmt, enter make upload PORT=\<path/to/device\>

##### <h5>After flashing:</h5>

After flashing the management chip you can run `make upload` without specifying the port, and it will be automatically found by the flasher and an attempt to upload will occur, when you see the attempt return to [Button press order](#button_press_order)

#### <h4 id="management_commands">Commands</h4>

- **reset** - Resets `ui` chip and then `net` chip. Accepts commands.
- **ui_upload** - Puts the management chip into `ui` chip upload mode. (STM32f05)
- **net_upload** - Puts the management chip into `net` chip upload mode. (ESP32S3)
- **debug** - Puts the management chip into debugging mode where serial messages from both `ui` and network chips are transmitted back to the usb interface. This is uni-directional from the management chip to the computer. However, all commands work during this mode.
- **debug_ui** - Only reads serial messages from the `ui` chip.
- **debug_net** - Only reads serial messages from the `net` chip.

<h4>Target Overview</h4>

- `all` - Builds the target bin, elf, and binary files

- `compile` - Builds the target bin, elf, and binary. Output firmware/build/mgmt

- `upload` - Uploads the build target to the Management chip using the our custom flasher tool with serial usb-c uploading. See [Python Flasher](#py_flasher)

- `upload-stflash` - Uploads the build target to the Management chip using the st-flash tools. Please see
[st-flash installation](#stflash_installation) for instructions on installing.

- `upload_cube_serial` - Uploads the build target to the Management chip using the STM32_Programmer_CLI with serial usb-c uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `upload_cube_swd` - Uploads the build target to the Management chip using the STM32_Programmer_CLI with st-link using SWD uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `docker` - Uses docker to compile, flash, and monitor the mgmt chip. By default it will compile the code. To change the functionality, pass a target of the Makefile into the parameter `mft` when being invoked.
    - Ex. `make docker mft=upload`
    - Ex. `make docker mft=monitor`

- `docker-clean` - Removes dockere image that was generated when running docker target.

- `clean` - Deletes the firmware/build/ui directory.

- `info` - Displays the build path, include files, object files, dependencies and vpaths used by the compiler.

<b>Source code</b>

All source code for the Management Chip can be found in firmware/mgmt. Adding C/C++ files in firmware/mgmt/src and firmware/mgmt/inc **does** require any changes in the makefile.

### <h3 id="ui"><b>User Interface</b></h3>

The User Interface chip is where most of the processing takes place. It utilizes a STM32F405 chip empowered by the STM HAL Library. To generate the baseline HAL Library code and Makefile, ST's CubeMX was used.

<b>Target Overview</b>

- `all` - Builds the target bin, elf, and binary files and uploads the code to the User Interface chip. Output firmware/build/ui

- `compile` - Builds the target bin, elf, and binary. Output firmware/build/ui

- `upload` - Uploads the build target to the User Interface chip using the st-flash tools. Please see
[st-flash installation](#stflash_installation) for instructions on installing.

- `upload_py` - Uploads the build target to the User Interface chip using the our custom flasher tool with serial usb-c uploading. See [Python Flasher](#py_flasher)

- `upload_cube_serial` - Uploads the build target to the User Interface chip using the STM32_Programmer_CLI with serial usb-c uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `upload_cube_swd` - Uploads the build target to the User Interface chip using the STM32_Programmer_CLI with st-link using SWD uploading. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `docker` - Uses docker to compile, flash, and monitor the ui chip. By default it will compile the code. To change the functionality, pass a target of the Makefile into the parameter `mft` when being invoked.
    - Ex. `make docker mft=upload`
    - Ex. `make docker mft=monitor`

- `monitor` - Opens a serial communication line with the management chip with a python script that allows for sending of commands.

- `openocd` - Opens a openocd debugging instance. See [Installation](#stm_installation) for instructions on installing openocd.

- `dirs` - Makes all directories required for by the Makefile to compile and upload.

- `docker-clean` - Removes dockere image that was generated when running docker target.

- `clean` - Deletes the firmware/build/ui directory.

- `info` - Displays the build path, include files, object files, dependencies and vpaths used by the compiler.

<b>Source code</b>

All source code for the User Interface can be found in firmware/ui, firmware/shared, and firmware/shared_inc. Adding C/C++ files in firmware/ui/src and firmware/ui/inc does not require any changes in the makefile.

Making a shared header between chips should be added into firmware/shared_inc.

### <h3 id="network"><b>Network</b></h3>

The network chip is a MOQ client using [quicr](https://github.com/Quicr/libquicr). The network chip communicates with the UI chip via UART serial communication.

We are leveraging the **esp-idf** framework.

<b>Target Overview</b>

- `all` - Runs the `compile` and `upload` targets.

- `compile` - Compiles the net code using the command `idf.py compile`

- `upload` - Uploads the net code using openocd

- `upload_py` - Uploads the build target to the Network chip using the our custom flasher tool with serial usb-c uploading. See [Python Flasher](#py_flasher)

- `upload_esptool` - Uploads the network chip code by first sending a command to the mgmt chip to put the board into **net upload** mode. Then uses `esptool.py upload`

- `docker` - Uses docker to compile, flash, and monitor the ui chip. By default it will compile the code. To change the functionality, pass a target of the Makefile into the parameter `mft` when being invoked.
    - Ex. `make docker mft=upload`
    - Ex. `make docker mft=monitor`

- `monitor` - Opens a serial communication line with the management chip in debug mode using screen.

- `py_monitor` - Opens a serial communication line with the management chip with a python script that allows for sending of commands.

- `clean` - Deletes the firmware/net/build directory.

- `fullclean` - Invokes idf.py fullclean.

- `docker-clean` - Removes dockere image that was generated when running docker target.

<b>Source code</b>

All source code for the network chip can be found in `firmware/net`, `firmware/shared`, and `firmware/shared_inc`.

## <h2 id="stm_installation">Installation - STM32 Toolchain</h2>

The following tools are used for Management and User Interface.
- [Make](#ui_make) \[required]
- [ARM GNU Toolchain](#arm-gnu) \[required]
- [Python3](https://www.python.org/downloads/) \[optional]
- [ST-Flash](#stflash) \[optional]
- [STM32_Programmer_CLI](stm_prog_cli) \[optional]
- [OpenOCD](openocd) \[optional]

<b id="ui_make">Make \[required]</b>

Make is used for compiling and uploading automation for each chip firmware.

<i>Debian</i>

```bash
sudo apt install make
```

<i>Windows</i>

- Download make.exe from [Source Forge](https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81.exe/download?use_mirror=phoenixnap&download=)

- Save it to C:/bin or where ever you want it. Then add the location to the your path.

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

```sh
brew install --cask gcc-arm-embedded
```

<br/>

<b id="stflash">ST Flash</b>

<i>Debian</i>

```bash
sudo apt install stlink-tools
```

<i>Windows</i>

Download the and install the archive and put the binaries on your path.
https://github.com/stlink-org/stlink/releases

<i>MacOS</i>

```bash
brew install stm32flash
```

<b id="stm_prog_cli">STM32 Programmer CLI \[Optional]</b>

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

If you don't have python3.8 installed, which is used for arm-none-eabi-gdb, then you'll need to install it
```bash
sudo add-apt-repository ppa:deadsnakes/ppa
apt update
sudo apt install python3.8
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
## <h2 id="esp_installation">Installation - ESP32 Toolchain</h2>

This project leverages the usage of the Arduino Library for the network chip.

The following tools are used for the Network.
- [Make](#net_make) \[required]
- [esp-idf](#esp_idf) \[required]

<b id="net_make">Make \[required]</b>

Make is used for compiling and uploading automation for each chip firmware.

<i>Debian</i>

```bash
sudo apt install make
```

<i>Windows</i>

- Download make.exe from [Source Forge](https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81.exe/download?use_mirror=phoenixnap&download=)

- Save it to OS bin directory or wherever you want it. Then add the location to the your path.

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


<h2 id="additional_notes">Additional Notes</h2>
<i>Linux</i>

- You will need to be in the dialout group
- You will need to be in the docker group if you want to use the docker targets.
- At any point if you're including or using any additional STM32 HAL library files such as `stm32f4xx_hal_sd.h` then you must add the respective source file `stm32f4xx_hal_sd.c` to C sources in the appropriate makefile.
- New Assembly source files need to be added to the ASM sources line in the approriate makefile.

## <h2 id="tools">Tools</h2>

### <h3 id="py_flasher"><b>Python Flasher</b></h3>
A firmware flashing tool designed to work with Hactar by collating STM32 and
ESP32 flashing specifications.

#### Requirements

- pyserial - ```pip install pyserial```

<h4>How to use</h4>
Generally the flasher is used automatically in the Makefile. However, you can flash whatever binary you want onto a chip that the flasher is designed for by running it as python script in the firmware/flasher folder.

ex.

```sh
python3 flasher.py --port=/dev/ttyUSB0 --baud=115200 --chip="<chip>" -bin="./build/app.bin"
```

You can omit passing a port and the flasher will attempt to find a Hactar board by searching your active usb serial ports.

```sh
python3 flasher.py --baud=115200 --chip="ui" -bin="./build/ui.bin"
```

### <h3 id="serial_monitor"><b>Python Serial Monitor</b></h3>

The serial monitor is a very simple serial monitor that allows a user to send commands to the mgmt chip as well as read serial logs. Located in firmware/tools

Requirements
- pyserial - ```pip install pyserial```

This monitor can be used by the following command:
`python monitor.py \[port] \[baudrate]`

ex.

`python monitor.py /dev/ttyUSB0 115200`

### <h2 id="hactar_setup">Hactar Setup</h2>
<i>Display connector</i>

- Required hardware
    - 2.4 inch LCD Module from Waveshare
    - Connector for display and Hactar board.
- From the display match the connector's pins with the associated pins on the connector.
    - VCC -- VCC
    - GND -- GND
    - DIN -- DISP_DIN
    - CLK -- DISP_CLK
    - CS  -- DISP_CS
    - DC  -- DISP_DC
    - RST -- DISP_RST
    - BL  -- DISP_BL

<i>Management Chip</i>

- Prerequisites
    - USB-C Cable
    - Python3
        - PySerial - download using `pip3 install pyserial`
    - arm-none-eabi-g++
    - make
- Build the mgmt code by navigating to `hactar/firmware/mgmt` and entering `make compile`
- Plug in a USB-C
- Press and hold the BOOT button, press and release the RESET button, finally release the BOOT button.
- Upload the mgmt code by entering `make upload`
- After this you should see a couple of LED's light up

<i>Main Chip</i>

- Prerequisites
    - A programmed `management chip`
    - USB-C Cable
    - Python3
        - PySerial - download using `pip3 install pyserial`
    - arm-none-eabi-g++
    - make
- Plug in the USB-C cable to the Hactar board.
- Build the ui code by navigating to the `hactar/firmware/ui` folder and entering `make compile`
- Upload the ui code by entering `make upload_py`
    - The python script "flasher.py" is called to upload to the main chip.
    - NOTE - For some reason the Main STM32 chip doesn't like being put into bootloader mode this way. Fixing it is a WIP. You just need to keep trying... sorry.
- After finishing uploading the firmware to the main chip, the management chip will return to running mode.

<i>Network Chip</i>

- Prerequisites
    - A programmed `management chip`
    - USB-C Cable
    - esp-idf - must be on your path
    - make
- Plug in the USB-C cable to the Hactar board.
- Build the net code by navigating to the `hactar/firmware/net` folder and entering `make compile`
- Upload the net code by entering `make upload_py`
    - The python script "flasher.py" is called to upload to the net chip
    - NOTE - The size is quite large, takes about 4 minutes to upload.
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

## <h2 id="troubleshooting">Troubleshooting</h2>

### USB device disconnects shortly after connecting randomly

- CH340 chips are shared with a program called brltty which is for braille devices. Linux automatically will try to connect it to brltty and there for is in use, you can unplug and replug until it doesn't take control of the device or you can uninstall brltty if you don't need braille device.
    - ``` sudo apt remove brltty ```