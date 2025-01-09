use cpal::{
    traits::{DeviceTrait, HostTrait, StreamTrait},
    BufferSize,
};
use itertools::Itertools;
use minifb::{Key, MouseButton, Scale, Window, WindowOptions};
use rand::Rng;
use std::collections::HashMap;
use std::sync::mpsc;

type AudioPacket = Vec<u16>;

enum WorldToViewEvent {
    Click,
    Audio(AudioPacket),
}

enum ViewToWorldEvent {
    Show(&'static str, usize),
    StartRecording,
    StopRecording,
    Audio(AudioPacket),
}

enum ViewToControllerEvent {
    Click,
    Audio(AudioPacket),
}

enum ControllerToViewEvent {
    Show(&'static str, usize),
    StartRecording,
    StopRecording,
    Audio(AudioPacket),
}

#[derive(Clone)]
enum NetworkEvent {
    Click(&'static str),
    Audio(AudioPacket),
}

struct RGB(u32, u32, u32);

impl From<u32> for RGB {
    fn from(x: u32) -> RGB {
        RGB((x >> 16) & 0xff, (x >> 8) & 0xff, x & 0xff)
    }
}

impl From<RGB> for u32 {
    fn from(rgb: RGB) -> u32 {
        (rgb.0 << 16) | (rgb.1 << 8) | rgb.2
    }
}

impl RGB {
    fn blend(a: u32, b: u32, i: u32, n: u32) -> u32 {
        let a = Self::from(a);
        let b = Self::from(b);

        u32::from(Self(
            (a.0 * (n - i) / n) + (b.0 * i / n),
            (a.1 * (n - i) / n) + (b.1 * i / n),
            (a.2 * (n - i) / n) + (b.2 * i / n),
        ))
    }
}

const MIDPOINT: u16 = u16::MAX >> 1;
const SCALE: f32 = MIDPOINT as f32;

fn unfloat(sample: f32) -> u16 {
    ((sample + 1.0) * SCALE) as u16
}

fn refloat(sample: u16) -> f32 {
    ((sample as f32) / SCALE) - 1.0
}

struct World {
    name: &'static str,
    to_view: mpsc::Sender<WorldToViewEvent>,
    from_view: mpsc::Receiver<ViewToWorldEvent>,

    window: Window,
    color: u32,
    next_color: u32,
    step: u32,
    buffer: [u32; Self::BUFFER_SIZE],
    mouse_down: bool,

    audio_input: cpal::Stream,
    audio_output: cpal::Stream,
    audio_packet_send: mpsc::Sender<AudioPacket>,
}

impl World {
    const TITLE: &str = "Press ESC to exit";
    const WIDTH: usize = 240;
    const HEIGHT: usize = 320;
    const BUFFER_SIZE: usize = Self::WIDTH * Self::HEIGHT;
    const AUDIO_FRAME_MS: f32 = 10.0;

    fn new(
        name: &'static str,
        to_view: mpsc::Sender<WorldToViewEvent>,
        from_view: mpsc::Receiver<ViewToWorldEvent>,
    ) -> Self {
        // Set up the interaction window
        let mut window = Window::new(
            format!("[{}]: {}", name, Self::TITLE).as_str(),
            Self::WIDTH,
            Self::HEIGHT,
            WindowOptions {
                resize: false,
                scale: Scale::X2,
                ..WindowOptions::default()
            },
        )
        .expect("Unable to create the window");

        window.set_target_fps(60);
        window.set_background_color(0, 0, 0);

        let color: u32 = 0x00_00_00_00;
        let next_color = rand::thread_rng().gen::<u32>() & 0x00_ff_ff_ff;

        // Set up audio devices
        let host = cpal::default_host();
        let input_device = host
            .default_input_device()
            .expect("failed to find input device");
        let output_device = host
            .default_output_device()
            .expect("failed to find output device");

        let mut config: cpal::StreamConfig = input_device.default_input_config().unwrap().into();
        let packet_frames = (Self::AUDIO_FRAME_MS / 1_000.0) * config.sample_rate.0 as f32;
        let packet_samples = packet_frames as usize * config.channels as usize;
        config.buffer_size = BufferSize::Fixed(5 * (packet_frames as u32));

        let (audio_packet_send, audio_packet_recv) = mpsc::channel::<AudioPacket>();

        let to_view_clone = to_view.clone();
        let input_data_fn = move |data: &[f32], _: &cpal::InputCallbackInfo| {
            for packet in data.chunks(packet_samples) {
                let packet = packet.iter().copied().map(unfloat).collect();
                to_view_clone.send(WorldToViewEvent::Audio(packet)).unwrap();
            }
        };

        let output_data_fn = move |data: &mut [f32], _: &cpal::OutputCallbackInfo| {
            let mut data = data;

            // Results in slow, choppy audio because of injected samples
            while !data.is_empty() {
                let packet: Vec<f32> = match audio_packet_recv.try_recv() {
                    Ok(packet) => packet.iter().copied().map(refloat).collect(),
                    Err(mpsc::TryRecvError::Empty) => vec![0.0; packet_samples],
                    Err(mpsc::TryRecvError::Disconnected) => return,
                };

                let n = std::cmp::min(data.len(), packet.len());
                data[..n].copy_from_slice(&packet[..n]);
                data = &mut data[n..];
            }
        };

        let err_fn = |err| {
            eprintln!("audio error: {:?}", err);
        };

        let audio_input = input_device
            .build_input_stream(&config, input_data_fn, err_fn, None)
            .unwrap();
        let audio_output = output_device
            .build_output_stream(&config, output_data_fn, err_fn, None)
            .unwrap();

        audio_input.pause().unwrap();
        audio_output.play().unwrap();

        // Assemble the world
        Self {
            name,
            to_view,
            from_view,
            window,
            color,
            next_color,
            step: 0,
            buffer: [0; Self::BUFFER_SIZE],
            mouse_down: false,

            audio_input,
            audio_output,
            audio_packet_send,
        }
    }

    fn ready(&self) -> bool {
        self.window.is_open() && !self.window.is_key_down(Key::Escape)
    }

    fn run_one(&mut self) {
        // Check for events from the view logic
        match self.from_view.try_recv() {
            Ok(event) => self.handle_view(event),
            Err(mpsc::TryRecvError::Empty) => {}
            Err(mpsc::TryRecvError::Disconnected) => return,
        }

        // Detect clicks
        let mouse_down = self.window.get_mouse_down(MouseButton::Left);
        if mouse_down != self.mouse_down {
            if mouse_down {
                self.to_view.send(WorldToViewEvent::Click).unwrap();
            }

            self.mouse_down = mouse_down;
        }

        // Draw pretty colors
        const STEPS: u32 = 128;

        self.step += 1;
        if self.step >= STEPS {
            self.color = self.next_color;
            self.next_color = rand::thread_rng().gen();
            self.next_color &= 0x00_ff_ff_ff;
            self.step = 0;
        }

        let curr_color = RGB::blend(self.color, self.next_color, self.step, STEPS);
        self.buffer.fill(curr_color as u32);

        self.window
            .update_with_buffer(&self.buffer, 240, 320)
            .unwrap();
    }

    fn handle_view(&mut self, event: ViewToWorldEvent) {
        match event {
            ViewToWorldEvent::Show(name, count) => {
                println!(
                    "[{}] ViewToWorldEvent::Show name={} count={}",
                    self.name, name, count
                );
            }
            ViewToWorldEvent::StartRecording => {
                println!("[{}] ViewToWorldEvent::StartRecording", self.name);
                self.audio_input.play().unwrap();
            }
            ViewToWorldEvent::StopRecording => {
                println!("[{}] ViewToWorldEvent::StopRecording", self.name);
                self.audio_input.pause().unwrap();
            }
            ViewToWorldEvent::Audio(packet) => {
                println!(
                    "[{}] ViewToWorldEvent::Audio packet.len={}",
                    self.name,
                    packet.len()
                );
                self.audio_packet_send.send(packet).unwrap();
            }
        }
    }
}

struct View {
    name: &'static str,
    to_world: mpsc::Sender<ViewToWorldEvent>,
    from_world: mpsc::Receiver<WorldToViewEvent>,
    to_controller: mpsc::Sender<ViewToControllerEvent>,
    from_controller: mpsc::Receiver<ControllerToViewEvent>,
}

impl View {
    fn run(mut self) {
        loop {
            // Busy-wait version of select!()
            match self.from_world.try_recv() {
                Ok(event) => self.handle_world(event),
                Err(mpsc::TryRecvError::Empty) => {}
                Err(mpsc::TryRecvError::Disconnected) => return,
            }

            match self.from_controller.try_recv() {
                Ok(event) => self.handle_controller(event),
                Err(mpsc::TryRecvError::Empty) => {}
                Err(mpsc::TryRecvError::Disconnected) => return,
            }
        }
    }

    fn handle_world(&mut self, event: WorldToViewEvent) {
        match event {
            WorldToViewEvent::Click => {
                println!("[{}] WorldToViewEvent::Click", self.name);
                self.to_controller
                    .send(ViewToControllerEvent::Click)
                    .unwrap();
            }
            WorldToViewEvent::Audio(packet) => {
                println!(
                    "[{}] WorldToViewEvent::Audio packet.len={}",
                    self.name,
                    packet.len()
                );
                self.to_controller
                    .send(ViewToControllerEvent::Audio(packet))
                    .unwrap();
            }
        };
    }

    fn handle_controller(&mut self, event: ControllerToViewEvent) {
        match event {
            ControllerToViewEvent::Show(name, count) => {
                println!(
                    "[{}] ControllerToViewEvent::Show name={} count={}",
                    self.name, name, count
                );
                self.to_world
                    .send(ViewToWorldEvent::Show(name, count))
                    .unwrap();
            }
            ControllerToViewEvent::StartRecording => {
                println!("[{}] ControllerToViewEvent::StartRecording", self.name);
                self.to_world
                    .send(ViewToWorldEvent::StartRecording)
                    .unwrap();
            }
            ControllerToViewEvent::StopRecording => {
                println!("[{}] ControllerToViewEvent::StopRecording", self.name);
                self.to_world.send(ViewToWorldEvent::StopRecording).unwrap()
            }
            ControllerToViewEvent::Audio(packet) => {
                println!(
                    "[{}] ControllerToViewEvent::Audio packet.len={}",
                    self.name,
                    packet.len()
                );
                self.to_world.send(ViewToWorldEvent::Audio(packet)).unwrap();
            }
        };
    }
}

struct Controller {
    name: &'static str,

    to_view: mpsc::Sender<ControllerToViewEvent>,
    from_view: mpsc::Receiver<ViewToControllerEvent>,

    to_network: mpsc::Sender<NetworkEvent>,
    from_network: mpsc::Receiver<NetworkEvent>,

    network_interface: Option<(mpsc::Sender<NetworkEvent>, mpsc::Receiver<NetworkEvent>)>,

    count: HashMap<&'static str, usize>,
    recording_active: bool,
}

impl Controller {
    fn run(mut self) {
        loop {
            // Busy-wait version of select!()
            match self.from_view.try_recv() {
                Ok(event) => self.handle_view(event),
                Err(mpsc::TryRecvError::Empty) => {}
                Err(mpsc::TryRecvError::Disconnected) => return,
            }

            match self.from_network.try_recv() {
                Ok(event) => self.handle_network(event),
                Err(mpsc::TryRecvError::Empty) => {}
                Err(mpsc::TryRecvError::Disconnected) => return,
            }
        }
    }

    fn handle_view(&mut self, event: ViewToControllerEvent) {
        match event {
            ViewToControllerEvent::Click => {
                println!("[{}] ViewToControllerEvent::Click", self.name);

                // Notify other devices of the click
                let net_event = NetworkEvent::Click(self.name);
                self.handle_network(net_event.clone());
                self.to_network.send(net_event).unwrap();

                // Start or stop recording, as appropriate
                self.recording_active = !self.recording_active;
                if self.recording_active {
                    self.to_view
                        .send(ControllerToViewEvent::StartRecording)
                        .unwrap();
                } else {
                    self.to_view
                        .send(ControllerToViewEvent::StopRecording)
                        .unwrap();
                }
            }
            ViewToControllerEvent::Audio(packet) => {
                println!(
                    "[{}] ViewToControllerEvent::Audio packet.len={}",
                    self.name,
                    packet.len()
                );
                self.to_network.send(NetworkEvent::Audio(packet)).unwrap();
            }
        }
    }

    fn handle_network(&mut self, event: NetworkEvent) {
        match event {
            NetworkEvent::Click(name) => {
                println!("[{}] NetworkEvent::Click name={}", self.name, name);
                let count = self.count.entry(name).or_default();

                // Record the click
                *count += 1;
                self.to_view
                    .send(ControllerToViewEvent::Show(name, *count))
                    .unwrap();
            }
            NetworkEvent::Audio(packet) => {
                println!(
                    "[{}] NetworkEvent::Audio packet.len={}",
                    self.name,
                    packet.len()
                );
                self.to_view
                    .send(ControllerToViewEvent::Audio(packet))
                    .unwrap();
            }
        }
    }

    fn network_interface(
        &mut self,
    ) -> Option<(mpsc::Sender<NetworkEvent>, mpsc::Receiver<NetworkEvent>)> {
        self.network_interface.take()
    }
}

struct Device {
    name: &'static str,
    world: World,
    view: Option<View>,
    controller: Option<Controller>,
}

impl Device {
    fn new(name: &'static str) -> Self {
        let (world_to_view_send, world_to_view_recv) = mpsc::channel();
        let (view_to_world_send, view_to_world_recv) = mpsc::channel();
        let (view_to_controller_send, view_to_controller_recv) = mpsc::channel();
        let (controller_to_view_send, controller_to_view_recv) = mpsc::channel();
        let (controller_to_network_send, controller_to_network_recv) = mpsc::channel();
        let (network_to_controller_send, network_to_controller_recv) = mpsc::channel();

        let world = World::new(name, world_to_view_send, view_to_world_recv);

        let view = View {
            name,
            to_world: view_to_world_send,
            from_world: world_to_view_recv,
            to_controller: view_to_controller_send,
            from_controller: controller_to_view_recv,
        };

        let controller = Controller {
            name,
            to_view: controller_to_view_send,
            from_view: view_to_controller_recv,
            to_network: controller_to_network_send,
            from_network: network_to_controller_recv,
            network_interface: Some((network_to_controller_send, controller_to_network_recv)),
            count: HashMap::new(),
            recording_active: false,
        };

        Self {
            name,
            world,
            view: Some(view),
            controller: Some(controller),
        }
    }

    fn start_internals(&mut self) {
        let view = self.view.take().unwrap();
        let controller = self.controller.take().unwrap();

        std::thread::spawn(move || view.run());
        std::thread::spawn(move || controller.run());
    }

    fn world_ready(&self) -> bool {
        self.world.ready()
    }

    fn run_world_one(&mut self) {
        self.world.run_one();
    }

    fn network_interface(
        &mut self,
    ) -> Option<(mpsc::Sender<NetworkEvent>, mpsc::Receiver<NetworkEvent>)> {
        self.controller
            .as_mut()
            .map(|x| x.network_interface())
            .flatten()
    }
}

fn main() {
    let mut devices = [Device::new("a"), Device::new("b")];

    // Pipe the network from one device to the other
    let (to_a, from_a) = devices[0].network_interface().unwrap();
    let (to_b, from_b) = devices[1].network_interface().unwrap();
    std::thread::spawn(move || loop {
        // Busy-wait version of select!()
        match from_a.try_recv() {
            Ok(event) => to_b.send(event).unwrap(),
            Err(mpsc::TryRecvError::Empty) => {}
            Err(mpsc::TryRecvError::Disconnected) => return,
        }

        match from_b.try_recv() {
            Ok(event) => to_a.send(event).unwrap(),
            Err(mpsc::TryRecvError::Empty) => {}
            Err(mpsc::TryRecvError::Disconnected) => return,
        }
    });

    // Device internals run on separate threads
    for d in devices.iter_mut() {
        d.start_internals();
    }

    // UI has to run on the main thread
    while devices.iter().all(|d| d.world_ready()) {
        for d in devices.iter_mut() {
            d.run_world_one();
        }
    }
}
