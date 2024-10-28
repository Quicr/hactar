#![deny(missing_docs, warnings)]

//! This crate simply instantiates the Hactar application on the macOS platform.

use hactar_platform_macos::MacOsPlatform;
use hactar_ui::App;

#[allow(unused)] // XXX(RLB) Actually is used, by tokio
#[tokio::main]
async fn main() {
    App::new(MacOsPlatform::default()).run().await
}
