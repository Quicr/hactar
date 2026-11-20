#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

#[repr(C)]
pub enum HAL_LockTypeDef {
    HAL_UNLOCKED,
    HAL_LOCKED,
}
