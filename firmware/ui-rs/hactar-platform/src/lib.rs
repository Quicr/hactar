#![no_std]
#![deny(missing_docs, warnings)]

//! TODO(RLB) documentation

/// The [`Platform`] trait encapsulates the capabilities that we expect to be exposed by a device
/// that supports the Hactar UI application.
pub trait Platform {
    /// A human-readable name for the platform
    fn name() -> &'static str;
}
