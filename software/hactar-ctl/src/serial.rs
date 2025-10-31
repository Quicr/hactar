use anyhow::Result;
use std::time::Duration;

pub struct Serial {
    port: Box<dyn serialport::SerialPort>,
    tx_buff: Vec<u8>,
    rx_buff: Vec<u8>,
}

impl Serial {
    pub fn open(path: String, baud_rate: u32) -> Result<Self> {
        let port = serialport::new(path, baud_rate)
            .timeout(Duration::from_millis(1000))
            .open()?;

        Ok(Self {
            port: port,
            tx_buff: Vec::<u8>::new(),
            rx_buff: Vec::<u8>::new(),
        })
    }
}

impl crate::SerialPort for Serial {
    fn num_available(&mut self) -> u32 {
        return self
            .port
            .bytes_to_read()
            .expect("Failed to get bytes to read from serial port");
    }

    fn write(&mut self, data: &Vec<u8>) -> usize {
        let result = self.port.write(data);
        match result {
            Ok(n) => {
                return n;
            }
            Err(_) => return 0,
        }
    }

    fn read_byte(&mut self, timeout: Duration) -> Vec<u8> {
        return self.read_bytes(1, timeout);
    }

    fn read_bytes(&mut self, num_bytes: usize, timeout: Duration) -> Vec<u8> {
        self.port.set_timeout(timeout);

        let mut buff = Vec::with_capacity(num_bytes);
        let num_recv = self
            .port
            .read(buff.as_mut_slice())
            .expect("Failed to read from port");

        if num_recv < buff.len() {
            buff.truncate(num_recv);
        }

        return buff;
    }

    fn read_line(&mut self, timeout: Duration) -> Vec<u8> {
        self.port.set_timeout(timeout);

        let mut arr = [0u8; 1];
        let mut buff = Vec::new();

        loop {
            match self.port.read(&mut arr) {
                Ok(n) => {
                    if arr[0] == b'\n' || n == 0 {
                        return buff;
                    }
                    buff.push(arr[0]);
                }
                Err(ref e) if e.kind() == std::io::ErrorKind::TimedOut => {
                    return buff;
                }
                Err(_) => {
                    return Vec::new();
                }
            }
        }
    }

    fn flush(&mut self) {}
}
