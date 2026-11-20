#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
#[repr(C)]
pub struct GPIO_TypeDef {
    MODER: u32,
    OTYPER: u32,
    OSPEEDR: u32,
    PUPDR: u32,
    IDR: u32,
    ODR: u32,
    BSRR: u32,
    LCKR: u32,
    AFR: [u32; 2],
}

#[repr(C)]
pub struct gpio_t {
    pub port: *mut GPIO_TypeDef,
    pub pin: u16,
}

#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
pub type GPIO_PinState = u32;
pub const GPIO_PIN_RESET: GPIO_PinState = 0;
pub const GPIO_PIN_SET: GPIO_PinState = 1;

#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
unsafe extern "C" {
    pub fn led_r() -> gpio_t;
    pub fn led_g() -> gpio_t;
    pub fn led_b() -> gpio_t;

    pub fn HAL_GPIO_ReadPin(GPIOx: *mut GPIO_TypeDef, GPIO_Pin: u16) -> GPIO_PinState;
    pub fn HAL_GPIO_WritePin(GPIOx: *mut GPIO_TypeDef, GPIO_Pin: u16, PinState: GPIO_PinState);
    pub fn HAL_GPIO_TogglePin(GPIOx: *mut GPIO_TypeDef, GPIO_Pin: u16);
}
