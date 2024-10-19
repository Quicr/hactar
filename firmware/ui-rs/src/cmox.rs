#![allow(non_snake_case)]
#![allow(non_camel_case_types)]

use defmt::*;
use embassy_stm32::{crc::Crc, peripherals::CRC};

type opaque = ();
type cmox_init_target_t = u32;
type cmox_init_retval_t = u32;

#[repr(C)]
struct cmox_init_arg_t {
    target: cmox_init_target_t,
    arg: *const u8,
}

#[repr(C)]
#[derive(Default)]
pub struct Info {
    pub version: u32,
    pub build: [u32; 7],
}

type cmox_hash_retval_t = u32;
type cmox_hash_algoStruct_st = opaque;
type cmox_hash_algo_t = *const cmox_hash_algoStruct_st;

extern "C" {
    static CMOX_SHA256_ALGO: cmox_hash_algo_t;

    fn cmox_initialize(init_arg: *mut cmox_init_arg_t) -> cmox_init_retval_t;

    fn cmox_getInfos(info: *mut Info);

    fn cmox_hash_compute(
        algo: cmox_hash_algo_t,
        plaintext: *const u8,
        plaintext_len: usize,
        digest: *mut u8,
        expected_digest_len: usize,
        computed_digest_len: *mut usize,
    ) -> cmox_hash_retval_t;
}

// XXX(RLB) If we want to recreate how things flow in C, we should pass a reference to the
// Peripherals object to `cmox_initialize()`, which will cause it to end up here, and then we can
// use it to initialize the CRC peripheral.
#[no_mangle]
extern "C" fn cmox_ll_init(_arg: *const opaque) -> cmox_init_retval_t {
    const CMOX_INIT_SUCCESS: cmox_init_retval_t = 0x00000000;
    CMOX_INIT_SUCCESS
}

pub fn init(crc: CRC) -> bool {
    const CMOX_INIT_TARGET_F4: cmox_init_target_t = 0x46340000;
    const CMOX_INIT_SUCCESS: cmox_init_retval_t = 0x00000000;

    // CMOX on STM32F4 requires the CRC peripheral to be initialized. In the C code, this is done
    // inside `cmox_ll_init(), but it is safe and simpler to do it before initialization.
    Crc::new(crc);

    let mut init_arg: cmox_init_arg_t = cmox_init_arg_t {
        target: CMOX_INIT_TARGET_F4,
        arg: core::ptr::null(),
    };

    let rv = unsafe { cmox_initialize(&mut init_arg) };
    rv == CMOX_INIT_SUCCESS
}

pub fn get_info() -> Info {
    let mut info = Info::default();
    unsafe { cmox_getInfos(&mut info) };
    info
}

pub fn sha256(data: &[u8], hash: &mut [u8; 32]) {
    const CMOX_HASH_SUCCESS: cmox_hash_retval_t = 0x00020000;

    let rv = unsafe {
        cmox_hash_compute(
            CMOX_SHA256_ALGO,
            data.as_ptr(),
            data.len(),
            hash.as_mut_ptr(),
            hash.len(),
            core::ptr::null_mut(),
        )
    };

    if rv != CMOX_HASH_SUCCESS {
        error!("Hash operation failed: {:08x}", rv);
        hash.fill(0);
    }
}
