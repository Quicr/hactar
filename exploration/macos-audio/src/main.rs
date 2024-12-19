//! Feeds back the input stream directly into the output stream.
//!
//! Assumes that the input and output devices can use the same stream configuration and that they
//! support the f32 sample format.
//!
//! Uses a delay of `LATENCY_MS` milliseconds in case the default input and output streams are not
//! precisely synchronised.

use clap::Parser;
use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use itertools::Itertools;
use std::io::{stdin, Read};
use std::sync::mpsc;

#[derive(Parser, Debug)]
#[command(version, about = "CPAL feedback example", long_about = None)]
struct Opt {
    /// Specify the delay between input and output
    #[arg(short, long, value_name = "DELAY_MS", default_value_t = 150.0)]
    latency: f32,
}

const MIDPOINT: u16 = u16::MAX >> 1;
const SCALE: f32 = MIDPOINT as f32;

fn unfloat(sample: f32) -> u16 {
    ((sample + 1.0) * SCALE) as u16
}

fn refloat(sample: u16) -> f32 {
    ((sample as f32) / SCALE) - 1.0
}

fn main() -> anyhow::Result<()> {
    let opt = Opt::parse();
    let host = cpal::default_host();

    // Find devices.
    let input_device = host
        .default_input_device()
        .expect("failed to find input device");

    let output_device = host
        .default_output_device()
        .expect("failed to find output device");

    // We'll try and use the same configuration between streams to keep it simple.
    let config: cpal::StreamConfig = input_device.default_input_config()?.into();

    // Package audio into frames.
    let packet_frames = (opt.latency / 1_000.0) * config.sample_rate.0 as f32;
    let packet_samples = packet_frames as usize * config.channels as usize;

    // Channels between threads
    let (in_send, in_recv) = mpsc::channel::<u16>(); // Input stream to packetizer
    let (pkt_send, pkt_recv) = mpsc::channel::<Vec<u16>>(); // Packetizer to depacketizer
    let (out_send, out_recv) = mpsc::channel::<u16>(); // Input stream to packetizer

    // Fill the samples with 0.0 equal to the length of the delay.
    for _ in 0..packet_samples {
        // The ring buffer has twice as much space as necessary to add latency here,
        // so this should never fail
        out_send.send(MIDPOINT).unwrap();
    }

    let input_data_fn = move |data: &[f32], _: &cpal::InputCallbackInfo| {
        for &sample in data {
            let sample = unfloat(sample);
            in_send.send(sample).unwrap();
        }
    };

    let packetize_fn = move || {
        for chunk in &in_recv.iter().chunks(packet_samples) {
            let packet: Vec<u16> = chunk.collect();
            pkt_send.send(packet).unwrap();
        }
    };

    let depacketize_fn = move || {
        for sample in pkt_recv.iter().flatten() {
            out_send.send(sample).unwrap();
        }
    };

    let output_data_fn = move |data: &mut [f32], _: &cpal::OutputCallbackInfo| {
        for sample in data {
            let sample_u16 = out_recv.recv().unwrap();
            *sample = refloat(sample_u16);
        }
    };

    // Spawn the packetizer and depacketizer
    std::thread::spawn(packetize_fn);
    std::thread::spawn(depacketize_fn);

    // Build streams.
    let input_stream = input_device.build_input_stream(&config, input_data_fn, err_fn, None)?;
    let output_stream = output_device.build_output_stream(&config, output_data_fn, err_fn, None)?;

    // Play the output streams.
    input_stream.pause()?;
    output_stream.play()?;

    // Press space to start or stop recording.
    let mut stdin_handle = stdin().lock();
    let mut playing = false;
    loop {
        let mut byte = [0_u8];
        stdin_handle.read_exact(&mut byte).unwrap();

        if byte[0] != 0x20 {
            continue;
        }

        playing = !playing;
        if playing {
            input_stream.play()?;
        } else {
            input_stream.pause()?;
        }
    }
}

fn err_fn(err: cpal::StreamError) {
    eprintln!("an error occurred on stream: {}", err);
}
