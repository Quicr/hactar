#![no_std]
#![deny(missing_docs, warnings)]

//! TODO(RLB) documentation

use hactar_platform::Platform;

/// TODO(RLB) documentation
pub struct App<P> {
    #[allow(unused)]
    platform: P,
}

impl<P> App<P>
where
    P: Platform,
{
    /// TODO(RLB) documentation
    pub fn new(platform: P) -> Self {
        Self { platform }
    }

    /// TODO(RLB) documentation
    pub async fn run(self) -> ! {
        loop {
            // TODO(RLB) put application logic here
        }
    }
}
