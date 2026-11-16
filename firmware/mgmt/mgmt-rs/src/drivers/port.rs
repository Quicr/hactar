#![no_std]

#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
#[repr(C)]
unsafe extern "C" {

    pub fn A() -> *mut GPIO_TypeDef;
    pub fn B() -> *mut GPIO_TypeDef;
    pub fn C() -> *mut GPIO_TypeDef;
}
