use anyhow::Result;
use std::time::Duration;
use tokio::io::{AsyncBufReadExt, BufReader};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::time::timeout;
use tokio_serial::{SerialPort, SerialPortBuilderExt};

/// TokioSerialPort - tokio-serial implementation for native platforms
pub struct TokioSerialPort {
    port: tokio_serial::SerialStream,
}

impl TokioSerialPort {
    // TODO open serial ports that are even parity
    /// Open a serial port using tokio-serial
    pub async fn open(path: &str, baud_rate: u32) -> Result<Self> {
        let port = tokio_serial::new(path, baud_rate)
            .data_bits(tokio_serial::DataBits::Eight)
            .stop_bits(tokio_serial::StopBits::One)
            .parity(tokio_serial::Parity::None)
            .open_native_async()?;

        // Give the device a moment to initialize
        tokio::time::sleep(Duration::from_millis(100)).await;

        Ok(Self { port })
    }
}

#[async_trait::async_trait(?Send)]
impl crate::SerialPort for TokioSerialPort {
    async fn write(&mut self, data: &[u8]) -> Result<()> {
        self.port.write_all(data).await?;
        Ok(())
    }

    async fn num_available(self) -> Result<u32> {
        return Ok(self
            .port
            .bytes_to_read()
            .expect("Failed to get bytes waiting in port"));
    }

    async fn read_bytes(&mut self, num_bytes: usize, timeout_ms: u64) -> Result<Vec<u8>> {
        let mut buff: Vec<u8> = Vec::with_capacity(num_bytes);

        let result = timeout(Duration::from_millis(timeout_ms), self.port.read(&mut buff)).await;

        match result {
            Ok(read_result) => match read_result {
                Ok(n) if n > 0 => {
                    buff.truncate(n);
                    return Ok(buff);
                }
                Ok(_) => return Ok(buff),
                Err(e) => {
                    return Err(e.into());
                }
            },
            Err(_) => return Ok(buff), // timeout
        }
    }

    async fn read_line(&mut self, timeout_ms: u64) -> Result<Vec<u8>> {
        let mut buff = vec![0u8; 1024];
        let mut reader = BufReader::new(&mut self.port);

        let result = timeout(
            Duration::from_millis(timeout_ms),
            reader.read_until(b'\n', &mut buff),
        )
        .await;

        match result {
            Ok(read_result) => match read_result {
                Ok(mut n) if n > 0 => {
                    if buff[n - 1] != b'\n' {
                        buff.push(b'\n');
                        n += 1;
                    }

                    buff.truncate(n);

                    return Ok(buff);
                }
                Ok(_) => {
                    // EOF, needs a new line
                    buff.push(b'\n');
                    return Ok(buff);
                }
                Err(e) => {
                    return Err(e.into());
                }
            },
            Err(_) => {
                buff.push(b'\n');
                return Ok(buff);
            }
        };
    }

    async fn read_with_timeout(&mut self, timeout_ms: u32) -> Result<Vec<u8>> {
        let mut buffer = vec![0u8; 1024];

        match timeout(
            Duration::from_millis(timeout_ms as u64),
            self.port.read(&mut buffer),
        )
        .await
        {
            Ok(Ok(n)) if n > 0 => {
                buffer.truncate(n);
                Ok(buffer)
            }
            Ok(Ok(_)) => Ok(Vec::new()), // EOF
            Ok(Err(e)) => Err(e.into()),
            Err(_) => Ok(Vec::new()), // Timeout
        }
    }

    async fn flush(&mut self) -> Result<()> {
        self.port.flush().await?;
        Ok(())
    }
}
