#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

use crate::drivers::hal_defs;
use core::ffi::c_void;

#[repr(C)]
pub struct DMA_Channel_TypeDef {
    CCR: u32,
    CNDTR: u32,
    CPAR: u32,
    CMAR: u32,
}

#[repr(C)]
pub struct DMA_TypeDef {
    ISR: u32,
    IFCR: u32,
}

#[repr(C)]
pub struct DMA_InitTypeDef {
    Direction: u32,
    PeriphInc: u32,
    MemInc: u32,
    PeriphDataAlignment: u32,
    MemDataAlignment: u32,
    Mode: u32,
    Priority: u32,
}

#[repr(C)]
pub enum HAL_DMA_StateTypeDef {
    HAL_DMA_STATE_RESET,
    HAL_DMA_STATE_READY,
    HAL_DMA_STATE_BUSY,
    HAL_DMA_STATE_TIMEOUT,
}

#[repr(C)]
pub struct DMA_HandleTypeDef {
    Instance: *mut DMA_Channel_TypeDef,
    Init: DMA_InitTypeDef,
    Lock: hal_defs::HAL_LockTypeDef,
    State: HAL_DMA_StateTypeDef,
    Parent: *mut c_void,
    XferCpltCallback: fn(*mut DMA_HandleTypeDef) -> c_void,
    XferHalfCpltCallback: fn(*mut DMA_HandleTypeDef) -> c_void,
    XferErrorCallback: fn(*mut DMA_HandleTypeDef) -> c_void,
    XferAbortCallback: fn(*mut DMA_HandleTypeDef) -> c_void,
    ErrorCode: u32,
    DmaBaseAddress: *mut DMA_TypeDef,
    ChannelIndex: u32,
}
