fn main() {
    // Include basic scaffolding
    println!("cargo:rustc-link-arg-bins=--nmagic");
    println!("cargo:rustc-link-arg-bins=-Tlink.x");
    println!("cargo:rustc-link-arg-bins=-Tdefmt.x");

    // Link the STM crypto library
    println!("cargo:rustc-link-search=lib");
    println!("cargo:rustc-link-lib=STM32Cryptographic_CM4");
}
