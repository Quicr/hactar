Hactar UI Firmware
==================

## Quickstart

```
# Prerequisites
> rustup install thumbv7em-none-eabi
> cargo install probe-rs-tools --locked

# To build and run the macOS app
> cd hactar-ui-macos
> cargo run

# To build and run the STM32 app
> cd hactar-ui-stm32
> cargo run
```

## Crates and Dependencies

This workspace comprises several crates that are used together to build the
firmware for the Hactar UI chip:

* `hactar-platform`: Core traits that abstract over the capabilities of the
  device, allowing for instantiation on an actual Hactar device or a macOS
  device
  * `hactar-platform-stm32`: Instantiation of the platform on STM32
  * `hactar-platform-macos`: Instantiaiton of the platform on macOS
  * `hactar-platform-mock`: A mock platform for testing
* `hactar-ui`: The Hactar UI application, relying only on the platform
  abstraction, not specific device details
  * `hactar-ui-stm32`: Instantiation of the application on STM32
  * `hactar-ui-macos`: Instantiation of the application on macOS
  * `hactar-ui-mock`: A mock application for testing
* `mls-rs-crypto-cmox`: A crypto provider for `mls-rs` based on the STM CMOX
  cryptographic library
  * `cmox-sys`: An unsafe wrapper around the CMOX library.

The following diagram illustrates the dependency relationships between these
crates.  Generally, the device-specific crates depend on the generic crates,
specializing them to a specific device.

```
            ui-stm32                ui-macos
             ^   ^                   ^   ^
             |   |                   |   |
             |   +---------+---------+   |
             |             |             |
             |            ui             |
             |             ^             |
         platform-stm32    |     platform-macos
          ^      ^         |         ^
          |      |         |         |
   +------+      +---------+---------+
   |                       |
mls-cmox                platform
   ^       
   |       
cmox-sys   
```

## Working in this Framework

Do not run `cargo build` or `cargo run` at the root of the workspace.  Nothing
bad will happen, but the stm32 build will fail.

The binaries produced by `ui-stm32` and `ui-macos` are the ones you will
want to run to actually create the user experience.  These crates should simply
be an instantiation of the application defined in the `ui` crate with a specific
platform instance.  The `platform` and `ui` crates may pull in dependencies, but
they should be able to be built for either platform.

The device-specific platform crates should pull in any device-specific
dependencies required to satisfy the API defined in `platform`.  The STM32
version in particular should depend on the relevant crates for HAL, etc.

Platform capabilities are added to the `platform` crate on an as-needed basis.
To add a new capability, you will want to add it to the relevant trait(s) in the
`platform` crate, and then you will need to add an implementation in the
device-specific crates.  (Even if it's a stub implementation.)
