fn main() {
    // Build the HAL and application libraries
    use cmake::Config;

    let dst = Config::new("../ui_static")
        .define("CMAKE_TOOLCHAIN_FILE", "../ui_static/toolchain.cmake")
        .build();

    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:rustc-link-lib=static=hal");
    println!("cargo:rustc-link-lib=static=app");

    // Build the startup assemby file
    cc::Build::new()
        .file("startup_stm32f405xx.s")
        .compile("startup");
    println!("cargo::rerun-if-changed=startup_stm32f405xx.s");

    // Linker configuration
    println!("cargo:rustc-link-arg-bins=--nmagic");
    println!("cargo:rustc-link-arg-bins=-L/Applications/ArmGNUToolchain/13.3.rel1/arm-none-eabi/arm-none-eabi/lib/thumb/v7+fp/hard/");
    println!("cargo:rustc-link-arg-bins=-lc");
    println!("cargo:rustc-link-arg-bins=-TSTM32F405RGTx_FLASH.ld");
}
