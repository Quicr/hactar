#![no_std]

#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
#[repr(C)]
pub struct UART_InitTypeDef {
    BaudRate: u32,
    WordLength: u32,
    StopBits: u32,
    Parity: u32,
    Mode: u32,
    HwFlowCtl: u32,
    OverSampling: u32,
    OneBitSampling: u32,
}

#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
#[repr(C)]
pub struct USART_Typedef {
    CR1: u32,
}

#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
unsafe extern "C" {

    // fn HAL_UART_Transmit(huart: *mut UART_HandleTypeDef, pData: *const u8, Size: u16, Timeout: u32);
    // fn HAL_UART_Receive(huart: *mut UART_HandleTypeDef, pData: *const u8, Size: u16, Timeout: u32);
}

pub fn test() -> u32 {
    return 0;
}
