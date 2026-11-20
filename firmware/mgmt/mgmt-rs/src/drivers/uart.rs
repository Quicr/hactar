#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

use crate::drivers::dma;
use crate::drivers::hal_defs;
use core::ffi::c_void;
use core::marker::{PhantomData, PhantomPinned};
use core::sync::atomic::{AtomicBool, Ordering};
use core::usize;

// Find in stm32f072xb.h
#[repr(C)]
pub struct UART_HandleTypeDef {
    _data: [u8; 0], // Evil stable rust hack
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

unsafe extern "C" {
    static mut huart1: UART_HandleTypeDef;
    static mut huart2: UART_HandleTypeDef;
    static mut huart3: UART_HandleTypeDef;

    pub fn HAL_UART_Transmit(
        huart: *mut UART_HandleTypeDef,
        p_data: *const u8,
        size: u16,
        timeout: u32,
    ) -> u32;

    pub fn HAL_UART_Receive(
        huart: *mut UART_HandleTypeDef,
        p_data: *const u8,
        size: u16,
        timeout: u32,
    ) -> u32;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum UartError {
    Error,
    Busy,
    Timeout,
    EmptyData,
    Unknown(u32),
}

#[derive(Clone, Copy, PartialEq, Eq)]
#[repr(usize)]
pub enum UartId {
    Uart1 = 0,
    Uart2,
    Uart3,
}
static NUM_UART: usize = 3;

impl UartId {
    fn handle(self) -> *mut UART_HandleTypeDef {
        match self {
            UartId::Uart1 => &raw mut huart1,
            UartId::Uart2 => &raw mut huart2,
            UartId::Uart3 => &raw mut huart3,
        }
    }
}

static TAKEN: [AtomicBool; NUM_UART] = [const { AtomicBool::new(false) }; NUM_UART];

pub struct Uart {
    handle: *mut UART_HandleTypeDef,
    id: UartId,
}

unsafe impl Send for Uart {}

impl Uart {
    pub fn take(id: UartId) -> Option<Self> {
        if TAKEN[id as usize].load(Ordering::Relaxed) {
            return None;
        } else {
            return Some(Uart {
                handle: id.handle(),
                id,
            });
        }
    }

    pub fn drop(&mut self) {
        TAKEN[self.id as usize].store(false, Ordering::Release);
    }

    pub fn transmit(&mut self, data: &[u8], timeout_ms: u32) -> Result<(), UartError> {
        if data.len() == 0 {
            return Err(UartError::EmptyData);
        }

        let res: u32 =
            unsafe { HAL_UART_Transmit(self.handle, data.as_ptr(), data.len() as u16, timeout_ms) };

        match res {
            0 => Ok(()),
            1 => Err(UartError::Error),
            2 => Err(UartError::Busy),
            3 => Err(UartError::Timeout),
            x => Err(UartError::Unknown(x)),
        }
    }
}
