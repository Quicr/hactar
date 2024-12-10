Hybrid UI Chip Firmware
=======================

In this directory, we are attempting to "hybridize" Rust and C for the UI chip
firmware, in the sense of moving parts of the current "C + C++ + GCC + Make"
scheme over to Rust and its toolchain.  The overall idea is to incrementally
replace parts with Rust code, verifying at each step that the overall app works
as expected:

* [X] Use Cargo + CMake instead of Makefiles
* [X] Replace the custom `.s` file with `#[entry]` and `#[interrupt]`
* [X] Use `build.rs` instead of `CMake`
* [ ] Streamline linker script and/or replace with auto-generated
* [ ] Move `main()` to Rust 
* [ ] Move `app_main()` to Rust
* [ ] Expose peripherals (screen, keyboard, etc.) through idiomatic Rust interfaces
* [ ] Write tests that verify the correct functioning of the peripherals

After those actions are complete, we should have a solid Rust platform on which
to build the rest of the application.
