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
    5. [Echo Server](#echo_server)
4. [Installation - STM32 Toolchain](#stm_installation)
5. [Installation - ESP32 Toolchain](#esp_installation)

<h2 id="where">Where To Find Things</h2>

- datasheets has all the datasheets for parts us

- docs has documents about this project

- hardware has the schematics and PCB designs

- productions has files used for manufacturing that are generated from the
stuff in hardware

- photos has image from the project as it progresses

- firmware has the code for testing and supporting the hardware

<h2 id="hardware">Hardware</h2>

<h3 id="hw_technologies">Technologies</h3>

- KiCAD

<h2 id="firmware">Firmware</h2>

The Firmware is split into 4 categories. Management, User Interface, Security, and Network.
- Management - STM32F072
- User Interface - STM32F05
- Security - STM32F05?
- Network - ESP32S3

<h3 id="management"><b>Management</b></h3>

WIP

<h3 id="ui"><b>User Interface</b></h3>

The User Interface chip is where most of the processing takes place. It utilizes a STM32F05 chip empowered by the STM HAL Library. To generate the baseline HAL Library code and Makefile, ST's CubeMX was used.

<b>Target Overview</b>

- `all` - Builds the target bin, elf, and binary files and uploads the code to the User Interface chip. Output firmware/build/ui

- `compile` - Builds the target bin, elf, and binary. Output firmware/build/ui

- `upload` - Uploads the build target to the User Interface chip using the STM32_Programmer_CLI. See [Installation](#stm_installation) for instructions on installing the STM32_Programmer_CLI.

- `program` - Runs the compile target, then the upload target.

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

You must run the `make install_board` prior to trying to run `compile`, `upload`, or `program` targets.

<b>Target Overview</b>

- `all` - Runs the `compile` and `upload` targets.

- `copy` - Copies the shared includes from `firmware/shared_inc` to `firmware/net/shared_inc` to appease Arduino's rigid source file locations rules.

- `compile` - Executes the `copy` target and then builds the Arduino target ino file with `Arduino-cli`. Output: `firmware/build/net`

- `upload` - Uploads the Arduino target to the Network chip using the Arduino-cli. See [installation](#esp_installation) for instructions on installing the `Arduino-cli`.

- `program` - Runs the compile target, then the upload target.

- `monitor` - Opens a serial monitor using `Arduino-cli`

- `install_board` - Updates the arduino core index specified by the arduino-cli.yaml file and installs the specified Arduino-core files for the given core.

- `clean` - Deletes the firmware/build/net directory.

<b>Source code</b>

All source code for the network chip can be found in `firmware/net` and `firmware/shared_inc`.

<b>Important Note</b> -- Do not add any additional files to `firmware/net/shared_inc` this is an important distinction, as those files are copied from `firmware/shared_inc` as to adhere to Arduino's rigid source file locations.

Adding a C/C++ file to firmware/net/inc and firmware/net/src does not require any additional changes to the `firmware/net/Makefile`. Similarly, any files added to `firmware/shared_inc` will be copied when the `copy` target is executed in the `firmware/net/makefile`.

<h3 id="security"><b>Security</b></h3>

WIP

<h3 id="echo_server"><b>Echo Server</b></h3>

<h2 id="stm_installation">Installation - STM32 Toolchain</h2>

The following tools are used for Management and User Interface.
- [Make](ui_make) \[required]
- [ARM GNU Toolchain](arm-gnu) \[required]
- [libusb](libusb) \[required - Linux only]
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

<b id="libusb">libusb \[required] - Linux only</b>

On linux you may need `libusb` installed. To install it run:
```
sudo apt install libusb-dev
```

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
- [Arduino CLI](arduino_cli) \[required]

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

<b id="arduino_cli">Arduino CLI \[required]</b>

<i>Debian</i>
```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/local/bin sh
```

<b>Note</b> - You can change the installation directory by changing the `BINDIR`

<i>Windows</i>

- Download and install the arduino-cli from https://arduino.github.io/arduino-cli/0.31/installation/


<i>MacOS</i>

```brew
brew update
brew install arduino-cli
```