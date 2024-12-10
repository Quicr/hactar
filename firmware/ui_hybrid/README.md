Hybrid UI Chip Firmware
=======================

In this directory, we are attempting to "hybridize" Rust and C for the UI chip
firmware, in the sense of moving parts of the current "C + C++ + GCC + Make"
scheme over to Rust and its toolchain.  The overall idea is to incrementally
replace parts with Rust code, verifying at each step that the overall app works
as expected:

* [X] Use Cargo + CMake instead of Makefiles
* [ ] Replace the custom `.s` file with `#[entry]` and `#[interrupt]`
* [ ] Use `build.rs` instead of `CMake`
* [ ] Streamline linker script and/or replace with auto-generated
* [ ] Move `main()` to Rust 
* [ ] Move `app_main()` to Rust
* [ ] Expose peripherals (screen, keyboard, etc.) through idiomatic Rust interfaces
* [ ] Write tests that verify the correct functioning of the peripherals

After those actions are complete, we should have a solid Rust platform on which
to build the rest of the application.

## Current Status: It's Complicated

At this point, the build chain is a bit complex, because the C code will not
link properly if it is packaged as a library before linking.  The `.o` files
need to be fed directly to the linker, and in the proper order (startup script
last).  To satisfy these constraints, we currently use CMake to orchestrate the
following process:

* Build an `OBJECT` library (really just a collection of `.o` files)
* Build the Rust code as a static library
* Build the startup ASM
* Link everything together into an ELF file

To build and run:

```
> cmake -B build -DCMAKE_TOOLCHAIN_FILE=etc/toolchain.cmake
> cmake --build build
> cmake --build build --target upload
```

To get rid of all this complexity, we will need to get rid of the custom ASM
file.  That file does two things: (1) define the main entry point, and (2)
define the interrupt handlers.  The Rust `cortex_m` crate should be able to
handle that, so once we migrate those two things to `#[entry]` and
`#[interrupt]` macros, we should be able to drive the whole build off of Cargo,
removing all the CMake files.
