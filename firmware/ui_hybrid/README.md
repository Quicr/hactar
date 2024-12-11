Hybrid UI Chip Firmware
=======================

This directory contains a setup for building a UI chip firmware that is a hybrid
of existing C code and Rust code.  The actual tooling is CMake and Cargo, with a
convenience Makefile to streamline common operations:

```
> make          # Build the firmware (including both C and Rust components)
> make upload   # Flash the firmware to the device, building it if necessary
> make clean    # Delete build artifacts
> make cclean   # Delete build artifacts and CMake build config
```

The firmware is pieced together from the following components:

* `hal` (static library, C): The STM-provided HAL files
* `hal_adapter` (object files, C): Source files created from STM-provided
  templates that instantiate our device.
* `app` (static library, C++): Our application code
* `ui_rs` (static library, Rust): Rust code
* `startup` (object file, ASM): STM-provided initialization logic

The C and ASM libraries are built via CMake; the Rust library is built by Cargo,
and the final ELF file is built from CMake.

Currently, Rust provides an `rs_main` method and it takes over the interrupt
handlers that were formerly in `stm32f4xx_it.c`.  The startup logic sets the
`rs_main` method as the entry point for the firmware, and then the Rust code
calls the existing `main` function in `main.c`.  The interrupt handlers have
exactly the same code as the C ones, just translated 1:1 to Rust.

