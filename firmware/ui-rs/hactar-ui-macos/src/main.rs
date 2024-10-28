#![deny(missing_docs, warnings)]

//! TODO(RLB) documentation

use hactar_platform_macos::MacOsPlatform;
use hactar_ui::App;

#[allow(unused)] // XXX(RLB) Actually is used, by tokio
#[tokio::main]
async fn main() {
    App::new(MacOsPlatform::default()).run().await
}
