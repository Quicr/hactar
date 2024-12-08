Hybrid UI Chip Firmware
=======================

In this directory, we are attempting to "hybridize" Rust and C for the UI chip
firmware, in the sense of moving parts of the current "C + C++ + GCC + Make"
scheme over to Rust and its toolchain.  Rough trajectory:

* [X] Build a binary using STM linker and startup scripts
* [ ] Build STM-provided driver code and link it into the executable
* [ ] Build the STM-based adaptation layer, and start it from a Rust `main()`,
  with an empty Rust `app_main()`.
* [ ] Build application code, replacing the Rust `app_main()`.
* [ ] Have Rust take over the `main()` function in the STM adaptation layer
* [ ] Have Rust take over the `app_main()` function in the application code
