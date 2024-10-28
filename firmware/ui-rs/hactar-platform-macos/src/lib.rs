#![deny(missing_docs, warnings)]

//! TODO(RLB) documentation

use hactar_platform::Platform;

/// TODO(RLB) documentation
#[derive(Default)]
pub struct MacOsPlatform;

impl Platform for MacOsPlatform {
    fn name() -> &'static str {
        "macos"
    }
}
