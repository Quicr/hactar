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

* `hal` (static library, C): The STM-provided HAL files, and source files
  created from STM-provided templates that instantiate our device.
* `app` (static library, C++): Our application code
* `ui_rs` (static library, Rust): Rust code
* `startup` (object file, ASM): STM-provided initialization logic

The C and Rust libraries are built via Cargo.  The C++ library and the final ELF
file are built via CMake

```
Drivers/**/*.c ---+
                  |
                  +---> build.rs --+
                  |                |
Core/Src/*.c -----+                |
                                   |
                                   +---> cargo --+
                                   |             |
src/*.rs --------------------------+             |
                                                 |
src/**/*.cc -----------> cmake ------------------+
                                                 |
                                                 +--> cmake --> ui_hybrid.elf
                                                 |
startup_stm32f405xx.s ---------------------------+
```

Currently, Rust provides an `rs_main` method and it takes over the interrupt
handlers that were formerly in `stm32f4xx_it.c`.  The startup logic sets the
`rs_main` method as the entry point for the firmware, and then the Rust code
calls the existing `main` function in `main.c`.  The interrupt handlers have
exactly the same code as the C ones, just translated 1:1 to Rust.

## Why so complicated?

The above scheme is more complicated than one might like.  Ideally, we could
build any required C/C++ in a `build.rs` build script, and have cargo assemble
an ELF file from that plus the relevant Rust code, something like:

```
Drivers/**/*.c ---+
                  |
Core/Src/*.c -----+---> build.rs --+
                  |                |
src/**/*.cc ------+                +---> cargo --> ui_hybrid.elf
                                   |
src/*.rs --------------------------+
```

There are two problems preventing this more elegant solution:

1. The entry point and interrupt handlers are set in the `startup_stm32f405xx.s`
   ASM file, and it won't link properly in this scheme.  Anything that builds
   via `build.rs` is input to the final `cargo` build process as a library,
   and since the startup ASM defines the entry point, if it's in a library,
   no code gets emitted at all (!)

2. Since that problem causes the final assembly to be done separately from the
   Cargo build, and C++ is so fragile, there are linking issues if Cargo builds
   the C++ and CMake links it.

The way out of this seems to be to solve the startup problem.  To do this, we
will probably need to employ one of the several Rust embedded frameworks out
there.  But since that might involve some experimentation and re-architecting,
we might need to stick with the more complicated solution for a little while.

## Steps to Simplification

1. Translate the C++ code to Rust.  This will remove the need for a final
   compile stage, so the native Rust tooling for wiring up entry points and
   interrupt handlers can be used.

2. Use the native rust tooling for entry points and interrupt handlers.
   Remove the manual ASM file that builds the interrupt vector.

These two steps will let us use `cargo` as the top-level build tooling:

```
Drivers/**/*.c ---+
                  |
                  +---> build.rs --+
                  |                |
Core/Src/*.c -----+                |
                                   |
                                   +---> cargo --> ui_hybrid.elf
                                   |
src/*.rs --------------------------+
```

The final step in the conversion (might take longer) will be:

3. Use a Rust-based HAL instead of the STM-provided one.  Delete the C HAL and
   the build.rs tooling used to build it.

This will finally get us to a pure Rust build:

```
src/*.rs --> cargo --> ui_hybrid.elf
```

