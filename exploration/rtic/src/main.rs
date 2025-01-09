//#![deny(unsafe_code)]
//#![deny(warnings)]
#![allow(dead_code)]
#![no_main]
#![no_std]

use panic_rtt_target as _;
use rtic::app;
use rtic_monotonics::systick::prelude::*;
use rtt_target::{rprintln, rtt_init_print};
use stm32f4xx_hal::{
    dma::*,
    gpio::{
        gpiob::{PB8, PB9},
        gpioc::{PC13, PC14, PC5},
        Output, Speed,
    },
    pac::{DMA2, SPI1},
    prelude::*,
    spi::*,
};

systick_monotonic!(Mono);

#[app(device = stm32f4xx_hal::pac, peripherals = true, dispatchers = [DCMI])]
mod app {
    use super::*;

    #[shared]
    struct Shared {}

    #[local]
    struct Local {
        screen: Screen,
        led: PC5<Output>,
        state: bool,
    }

    #[init]
    fn init(cx: init::Context) -> (Shared, Local) {
        // Setup clocks
        let rcc = cx.device.RCC.constrain();

        // Initialize the systick interrupt & obtain the token to prove that we did
        Mono::start(cx.core.SYST, 168_000_000);

        rtt_init_print!();
        rprintln!("i init");

        let clocks = rcc
            .cfgr
            .sysclk(168.MHz())
            // XXX(RLB) Unclear which of these if any make sense
            //.use_hse(12.MHz())
            //.bypass_hse_oscillator()
            //.require_pll48clk()
            //.pclk1(42.MHz())
            //.pclk2(84.MHz())
            .freeze();

        rprintln!("i setup clocks");

        // Get references to the GPIO devices
        let gpioa = cx.device.GPIOA.split();
        let gpiob = cx.device.GPIOB.split();
        let gpioc = cx.device.GPIOC.split();

        // Setup LED
        let mut led = gpioc.pc5.into_push_pull_output();
        led.set_high();

        // Setup the screen
        let screen = {
            let sck = gpioa
                .pa5
                .into_alternate()
                .speed(Speed::VeryHigh)
                .internal_pull_up(true);
            let mosi = gpioa.pa7.into_alternate().speed(Speed::VeryHigh);

            let mode = Mode {
                polarity: Polarity::IdleLow,
                phase: Phase::CaptureOnFirstTransition,
            };

            let spi = Spi::new(
                cx.device.SPI1,
                (sck, NoMiso::new(), mosi),
                mode,
                3.MHz(),
                &clocks,
            )
            .use_dma()
            .tx();

            let stream = StreamsTuple::new(cx.device.DMA2).3;
            let buffer = cortex_m::singleton!(: [u8; Screen::WRITE_BUFFER_SIZE] = [0; Screen::WRITE_BUFFER_SIZE]).unwrap();

            Screen {
                state: Some(ScreenState {
                    spi,
                    stream,
                    buffer,
                }),
                cs: gpioc.pc14.into_push_pull_output(),
                rst: gpiob.pb9.into_push_pull_output(),
                dc: gpioc.pc13.into_push_pull_output(),
                bl: gpiob.pb8.into_push_pull_output(),
                view_width: 0,
                view_height: 0,
            }
        };

        rprintln!("i setup peripherals");

        // Schedule the main task
        //fib::spawn().ok();
        screen_blink::spawn().ok();

        (
            Shared {},
            Local {
                screen,
                led,
                state: false,
            },
        )
    }

    #[task(local = [screen])]
    async fn screen_blink(cx: screen_blink::Context) {
        rprintln!("! screen_blink");
        let screen = cx.local.screen;
        screen.init().await.ok();

        screen.set_backlight(true);

        const RED: u16 = 0xF800;
        const GREEN: u16 = 0x07E0;
        const BLUE: u16 = 0x001F;

        rprintln!("! drawing");
        for color in [RED, GREEN, BLUE].iter().cycle() {
            rprintln!("! fill");
            screen.fill_screen(*color).ok();
            Mono::delay(1000.millis()).await;
        }
        rprintln!("! drew");

        /*
        let mut state = false;
        loop {
            rprintln!("! blink");
            screen.set_backlight(state);
            state = !state;
            Mono::delay(1000.millis()).await;
        }
        */
    }

    #[task]
    async fn fib(_cx: fib::Context) {
        fib_test::<u32>(34).await;
        fib_test::<u64>(34).await;
        fib_test::<f32>(32).await;
        fib_test::<f64>(32).await;

        blink::spawn().ok();
    }

    #[task(local = [led, state])]
    async fn blink(cx: blink::Context) {
        loop {
            rprintln!("blink");
            if *cx.local.state {
                cx.local.led.set_high();
                *cx.local.state = false;
            } else {
                cx.local.led.set_low();
                *cx.local.state = true;
            }
            Mono::delay(1000.millis()).await;
        }
    }
}

async fn fib_test<'d, T>(n: usize)
where
    T: core::ops::Add<T, Output = T> + From<u8>,
{
    // Compute fibonacci numbers
    let before = Mono::now();
    let _x = fib::<T>(n);
    let after = Mono::now();

    // Report results to the debug probe
    rprintln!(
        "{} {} -> {} ms",
        core::any::type_name::<T>(),
        n,
        (after - before).to_millis()
    );
}

fn fib<T>(n: usize) -> T
where
    T: core::ops::Add<T, Output = T> + From<u8>,
{
    if n > 2 {
        fib::<T>(n - 1) + fib::<T>(n - 2)
    } else {
        1_u8.into()
    }
}

const SOFTWARE_RESET: u8 = 0x01;
const POWER_CONTROL_A: u8 = 0xCB;
const POWER_CONTROL_B: u8 = 0xCF;
const TIMER_CONTROL_A: u8 = 0xE8;
const TIMER_CONTROL_B: u8 = 0xEA;
const POWER_ON_SEQUENCE_CONTROL: u8 = 0xED;
const PUMP_RATIO_COMMAND: u8 = 0xF7;
const POWER_CONTROL_VRH: u8 = 0xC0;
const POWER_CONTROL_SAP_BT: u8 = 0xC1;
const VCM_CONTROL_1: u8 = 0xC5;
const VCM_CONTROL_2: u8 = 0xC7;
const MEMORY_ACCESS_CONTROL: u8 = 0x36;
const PIXEL_FORMAT: u8 = 0x3A;
const FRAME_RATIO_CONTROL: u8 = 0xB1;
const DISPLAY_FUNCTION_CONTROL: u8 = 0xB6;
const GAMMA_FUNCTION_DISPLAY: u8 = 0xF2;
const GAMMA_CURVE_SELECTED: u8 = 0x26;
const POSITIVE_GAMMA_CORRECTION: u8 = 0xE0;
const NEGATIVE_GAMMA_CORRECTION: u8 = 0xE1;
const EXIT_SLEEP: u8 = 0x11;
const DISPLAY_ON: u8 = 0x29;
const ROTATION_CONTROL: u8 = 0x36;
const COLUMN_ADDRESS_SET: u8 = 0x2a;
const ROW_ADDRESS_SET: u8 = 0x2b;
const WRITE_TO_RAM: u8 = 0x2c;

const SCREEN_STARTUP_SEQUENCE: &[(u8, &[u8])] = &[
    (POWER_CONTROL_A, &[0x39, 0x2C, 0x00, 0x34, 0x02]),
    (POWER_CONTROL_B, &[0x00, 0xC1, 0x30]),
    (TIMER_CONTROL_A, &[0x85, 0x00, 0x78]),
    (TIMER_CONTROL_B, &[0x00, 0x00]),
    (POWER_ON_SEQUENCE_CONTROL, &[0x64, 0x03, 0x12, 0x81]),
    (PUMP_RATIO_COMMAND, &[0x20]),
    (POWER_CONTROL_VRH, &[0x23]),
    (POWER_CONTROL_SAP_BT, &[0x10]),
    (VCM_CONTROL_1, &[0x3E, 0x28]),
    (VCM_CONTROL_2, &[0x86]),
    (MEMORY_ACCESS_CONTROL, &[0x48]),
    (PIXEL_FORMAT, &[0x55]),
    (FRAME_RATIO_CONTROL, &[0x00, 0x18]),
    (DISPLAY_FUNCTION_CONTROL, &[0x08, 0x82, 0x27]),
    (GAMMA_FUNCTION_DISPLAY, &[0x00]),
    (GAMMA_CURVE_SELECTED, &[0x01]),
    (
        POSITIVE_GAMMA_CORRECTION,
        &[
            0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09,
            0x00,
        ],
    ),
    (
        NEGATIVE_GAMMA_CORRECTION,
        &[
            0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36,
            0x0F,
        ],
    ),
];

#[derive(Copy, Clone, Debug, PartialEq)]
enum Orientation {
    Portrait,
    FlippedPortrait,
    LeftLandscape,
    RightLandscape,
}

impl Orientation {
    const WIDTH: usize = 240;
    const HEIGHT: usize = 320;

    // XXX(RLB) These values need more descriptive names
    const MY: u8 = 0x80;
    const MX: u8 = 0x40;
    const MV: u8 = 0x20;
    const BGR: u8 = 0x08;

    const PORTRAIT_MODE: u8 = Self::MY | Self::BGR;
    const FLIPPED_PORTRAIT_MODE: u8 = Self::MX | Self::BGR;
    const LEFT_LANDSCAPE_MODE: u8 = Self::MV | Self::BGR;
    const RIGHT_LANDSCAPE_MODE: u8 = Self::MX | Self::MY | Self::MV | Self::BGR;

    fn width(&self) -> usize {
        match *self {
            Orientation::Portrait | Orientation::FlippedPortrait => Self::WIDTH,
            Orientation::LeftLandscape | Orientation::RightLandscape => Self::HEIGHT,
        }
    }

    fn height(&self) -> usize {
        match *self {
            Orientation::Portrait | Orientation::FlippedPortrait => Self::HEIGHT,
            Orientation::LeftLandscape | Orientation::RightLandscape => Self::WIDTH,
        }
    }
}

impl From<Orientation> for u8 {
    fn from(orientation: Orientation) -> u8 {
        match orientation {
            Orientation::Portrait => Orientation::PORTRAIT_MODE,
            Orientation::FlippedPortrait => Orientation::FLIPPED_PORTRAIT_MODE,
            Orientation::LeftLandscape => Orientation::LEFT_LANDSCAPE_MODE,
            Orientation::RightLandscape => Orientation::RIGHT_LANDSCAPE_MODE,
        }
    }
}

struct ScreenState {
    spi: Tx<SPI1>,
    stream: Stream3<DMA2>,
    buffer: &'static mut [u8; 1024],
}

struct Screen {
    state: Option<ScreenState>,

    cs: PC14<Output>, // Chip select
    rst: PB9<Output>, // Reset
    dc: PC13<Output>, // Data / command
    bl: PB8<Output>,  // Backlight

    view_width: usize,
    view_height: usize,
}

impl Screen {
    const WRITE_BUFFER_SIZE: usize = 1024;

    async fn init(&mut self) -> Result<(), Error> {
        // Chip select is always low, since there's only one chip on this SPI bus
        rprintln!("> chip select");
        self.cs.set_low();

        // Reset the screen
        rprintln!("> reset screen");
        self.rst.set_low();
        Mono::delay(50.millis()).await;
        self.rst.set_high();

        rprintln!("> software reset");
        self.write_command(SOFTWARE_RESET)?;
        Mono::delay(5.millis()).await;

        // Issue the magic incantation
        rprintln!("> startup sequence");
        for (command, data) in SCREEN_STARTUP_SEQUENCE {
            self.write_command(*command)?;
            self.write_data(*data)?;
        }

        rprintln!("> exit sleep");
        self.write_command(EXIT_SLEEP)?;
        Mono::delay(120.millis()).await;
        self.write_command(DISPLAY_ON)?;

        self.set_orientation(Orientation::Portrait)?;

        Ok(())
    }

    fn write_command(&mut self, command: u8) -> Result<(), Error> {
        self.dc.set_low();
        self.write(&[command])
    }

    fn write_data(&mut self, data: &[u8]) -> Result<(), Error> {
        self.dc.set_high();
        self.write(data)
    }

    // TODO: Allow async by storing Transfer -> wait(); release();
    // TODO: Handle data that is larger than the DMA buffer.
    fn write(&mut self, data: &[u8]) -> Result<(), Error> {
        const MAX_CHUNK_SIZE: usize = 1 << 15;

        // Take ownership of the SPI, DMA stream, and DMA buffer.  Note that this will panic if
        // `self.state` is None.
        let ScreenState {
            spi,
            stream,
            buffer,
        } = self.state.take().unwrap();

        // Copy the data into the buffer
        // TODO: Handle data that is larger than the DMA buffer.
        let len = core::cmp::min(data.len(), buffer.len());
        buffer[..len].copy_from_slice(&data[..len]);

        // Perform the transfer
        let mut transfer = Transfer::init_memory_to_peripheral(
            stream,
            spi,
            buffer,
            None,
            config::DmaConfig::default()
                .memory_increment(true)
                .fifo_enable(true)
                .fifo_error_interrupt(true)
                .transfer_complete_interrupt(true),
        );

        transfer.start(|_tx| {});
        transfer.wait();

        // Return ownership of the SPI/DMA materials to `self`
        let (stream, spi, buffer, ..) = transfer.release();
        self.state = Some(ScreenState {
            spi,
            stream,
            buffer,
        });

        Ok(())
    }

    fn set_backlight(&mut self, on: bool) {
        if on {
            self.bl.set_high();
        } else {
            self.bl.set_low();
        }
    }

    fn set_orientation(&mut self, orientation: Orientation) -> Result<(), Error> {
        self.write_command(ROTATION_CONTROL)?;
        self.write_data(&[u8::from(orientation)])?;
        self.view_width = orientation.width();
        self.view_height = orientation.height();
        Ok(())
    }

    fn fill_screen(&mut self, color: u16) -> Result<(), Error> {
        let x0 = 0_u16;
        let y0 = 0_u16;
        let x_last = self.view_width - 1;
        let y_last = 100; // self.view_height - 1;
        let col_data: [u8; 4] = [
            (x0 >> 8) as u8,
            (x0 & 0xff) as u8,
            (x_last >> 8) as u8,
            (x_last & 0xff) as u8,
        ];
        let row_data: [u8; 4] = [
            (y0 >> 8) as u8,
            (y0 & 0xff) as u8,
            (y_last >> 8) as u8,
            (y_last & 0xff) as u8,
        ];

        self.write_command(COLUMN_ADDRESS_SET)?;
        self.write_data(&col_data)?;

        self.write_command(ROW_ADDRESS_SET)?;
        self.write_data(&row_data)?;

        self.write_command(WRITE_TO_RAM)?;

        let total_pixel_bytes = 2 * Orientation::WIDTH * Orientation::HEIGHT;
        let n_chunks = total_pixel_bytes / Self::WRITE_BUFFER_SIZE;
        let chunk = unsafe {
            // Transmutation of array sizes is safe
            core::mem::transmute::<
                [[u8; 2]; Self::WRITE_BUFFER_SIZE / 2],
                [u8; Self::WRITE_BUFFER_SIZE],
            >([color.to_be_bytes(); Self::WRITE_BUFFER_SIZE / 2])
        };
        for _i in 0..n_chunks {
            self.write_data(&chunk)?;
        }

        Ok(())
    }

    /*
    fn write_dma(&self, buffer: &[u8]) {
        let tx = self.spi.use_dma().tx();
        static buffer: ScreenBuffer = ScreenBuffer::default();

        let mut transfer = Transfer::init_memory_to_peripheral(
            self.dma_stream,
            tx,
            &buffer,
            None,
            config::DmaConfig::default()
                .memory_increment(true)
                .fifo_enable(true)
                .fifo_error_interrupt(true)
                .transfer_complete_interrupt(true),
        );

        transfer.start(|_tx| {});
    }

    // TODO(RLB) Have a nicer interface than u16 for pixels, something like `struct
    // PixelBuffer<'a>(&'a [u16])`.
    async fn update_area(
        &mut self,
        x0: usize,
        y0: usize,
        x1: usize,
        y1: usize,
        pixels: &[u16],
    ) -> Result<(), Error> {
        if x0 >= self.view_width || y0 >= self.view_height || x1 <= x0 || y1 <= y0 {
            // TODO(RLB) make this fallible
            rprintln!("invalid parameters");
            unreachable!();
        }

        let x1 = core::cmp::min(x1, self.view_width);
        let y1 = core::cmp::min(y1, self.view_height);

        let width = x1 - x0;
        let height = y1 - y0;
        let total_pixels = (width as usize) * (height as usize);
        if pixels.len() != total_pixels {
            // TODO(RLB) fail
            rprintln!("invalid buffer");
            unreachable!();
        }

        // These ranges are inclusive
        let x_last = x1 - 1;
        let y_last = y1 - 1;
        let col_data: [u8; 4] = [
            (x0 >> 8) as u8,
            (x0 & 0xff) as u8,
            (x_last >> 8) as u8,
            (x_last & 0xff) as u8,
        ];
        let row_data: [u8; 4] = [
            (y0 >> 8) as u8,
            (y0 & 0xff) as u8,
            (y_last >> 8) as u8,
            (y_last & 0xff) as u8,
        ];

        // XXX(RLB) Create a stack-allocated buffer on the fly
        // XXX(RLB) If this buffer is too big, the stack overflows.
        let mut pixel_buffer = [0_u8; 512];
        for (i, b) in pixels.iter().map(|b| b.to_be_bytes()).flatten().enumerate() {
            pixel_buffer[i] = b;
        }

        let pixel_buffer = &pixel_buffer[..(2 * total_pixels)];

        const MAX_CHUNK_SIZE: usize = 1024;

        // Set the writable area
        self.write_command(COLUMN_ADDRESS_SET)?;
        self.write_data(&col_data)?;

        self.write_command(ROW_ADDRESS_SET)?;
        self.write_data(&row_data)?;

        self.write_command(WRITE_TO_RAM)?;
        self.write_data(&pixel_buffer)?;
        //self.spi.write(&pixel_buffer).await?;

        Ok(())
    }
    */
}
