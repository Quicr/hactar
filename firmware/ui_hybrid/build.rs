fn main() {
    cc::Build::new()
        .file("startup_stm32f405xx.s")
        .compile("startup");
    println!("cargo::rerun-if-changed=startup_stm32f405xx.s");

    println!("cargo:rustc-link-arg-bins=--nmagic");
    println!("cargo:rustc-link-arg-bins=-L/Applications/ArmGNUToolchain/13.3.rel1/arm-none-eabi//arm-none-eabi/lib/thumb/v7+fp/hard/");
    println!("cargo:rustc-link-arg-bins=-lc");
    println!("cargo:rustc-link-arg-bins=-TSTM32F405RGTx_FLASH.ld");
}
