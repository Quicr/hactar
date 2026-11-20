#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

use crate::drivers::dma;
use crate::drivers::hal_defs;
use core::ffi::c_void;

// Find in stm32f072xb.h
#[repr(C)]
pub struct USART_Typedef {
    CR1: u32,
    CR2: u32,
    CR3: u32,
    BRR: u32,
    GTPR: u32,
    RTOR: u32,
    RQR: u32,
    ISR: u32,
    ICR: u32,
    RDR: u16,
    RESERVED1: u16,
    TDR: u16,
    RESERVED2: u16,
}

// typedef struct
// {
//   __IO uint32_t CR1;    /*!< USART Control register 1,                 Address offset: 0x00 */
//   __IO uint32_t CR2;    /*!< USART Control register 2,                 Address offset: 0x04 */
//   __IO uint32_t CR3;    /*!< USART Control register 3,                 Address offset: 0x08 */
//   __IO uint32_t BRR;    /*!< USART Baud rate register,                 Address offset: 0x0C */
//   __IO uint32_t GTPR;   /*!< USART Guard time and prescaler register,  Address offset: 0x10 */
//   __IO uint32_t RTOR;   /*!< USART Receiver Time Out register,         Address offset: 0x14 */
//   __IO uint32_t RQR;    /*!< USART Request register,                   Address offset: 0x18 */
//   __IO uint32_t ISR;    /*!< USART Interrupt and status register,      Address offset: 0x1C */
//   __IO uint32_t ICR;    /*!< USART Interrupt flag Clear register,      Address offset: 0x20 */
//   __IO uint16_t RDR;    /*!< USART Receive Data register,              Address offset: 0x24 */
//   uint16_t  RESERVED1;  /*!< Reserved, 0x26                                                 */
//   __IO uint16_t TDR;    /*!< USART Transmit Data register,             Address offset: 0x28 */
//   uint16_t  RESERVED2;  /*!< Reserved, 0x2A                                                 */
// } USART_TypeDef;

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

#[repr(C)]
pub struct UART_AdvFeatureInitTypeDef {
    AdvFeatureInit: u32,
    TxPinLevelInvert: u32,
    RxPinLevelInvert: u32,
    DataInvert: u32,
    Swap: u32,
    OverrunDisable: u32,
    DMADisableonRxError: u32,
    AutoBaudRateEnable: u32,
    AutoBaudRateMode: u32,
    MSBFirst: u32,
}

#[repr(C)]
pub struct UART_HandleTypeDef {
    Instance: *mut USART_Typedef,
    Init: UART_InitTypeDef,
    AdvancedInit: UART_AdvFeatureInitTypeDef,
    pTxBuffPtr: *const u8,
    TxXferSize: u16,
    TxXferCount: u16,
    pRxBuffPtr: *const u8,
    RxXferSize: u16,
    RxXferCount: u16,
    Mask: u16,
    ReceptionType: i32,
    RxEventType: i32,
    RxISR: fn(*mut UART_HandleTypeDef) -> c_void,
    TxISR: fn(*mut UART_HandleTypeDef) -> c_void,
    hdmatx: *mut dma::DMA_HandleTypeDef,
    hdmarx: *mut dma::DMA_HandleTypeDef,
    Lock: hal_defs::HAL_LockTypeDef,
    gState: u32,
    RxState: u32,
    ErrorCode: u32,
}

unsafe extern "C" {
    pub fn HAL_UART_Transmit(
        huart: *mut UART_HandleTypeDef,
        pData: *const u8,
        Size: u16,
        Timeout: u32,
    );
    pub fn HAL_UART_Receive(
        huart: *mut UART_HandleTypeDef,
        pData: *const u8,
        Size: u16,
        Timeout: u32,
    );

    pub fn usb_uart() -> *mut UART_HandleTypeDef;
    pub fn ui_uart() -> *mut UART_HandleTypeDef;
    pub fn net_uart() -> *mut UART_HandleTypeDef;
}
