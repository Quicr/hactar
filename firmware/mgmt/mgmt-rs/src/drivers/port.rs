#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

use crate::drivers::gpio;

unsafe extern "C" {
    pub fn A() -> *mut gpio::GPIO_TypeDef;
    pub fn B() -> *mut gpio::GPIO_TypeDef;
    pub fn C() -> *mut gpio::GPIO_TypeDef;
}
