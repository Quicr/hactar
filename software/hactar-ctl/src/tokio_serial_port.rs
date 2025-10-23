use anyhow::Result;
use std::time::Duration;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::time::timeout;
use tokio_serial::SerialPortBuilderExt;

/// TokioSerialPort - tokio-serial implementation for native platforms
pub struct TokioSerialPort {
    port: tokio_serial::SerialStream,
}

impl TokioSerialPort {
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
