#include "SerialManager.hh"
#include "stm32f4xx_hal_uart.h"

SerialManager::SerialManager(UART_HandleTypeDef *uart_handler) :
    uart(uart_handler),
    rx_parsed_packets(10),
    tx_is_free(true),
    has_rx_data(false),
    tx_buffer(nullptr),
    rx_buffer(nullptr),
    rx_ring(512),
    rx_packet(nullptr),
    tx_buffer_sz(0),
    rx_buffer_sz(0)
{
    // THINK move to userinterface?
    // TODO remove magic number

    // Start receiving
    StartRTIRx(256);
}

SerialManager::~SerialManager()
{
    if (tx_buffer) delete [] tx_buffer;
    if (rx_buffer) delete [] rx_buffer;

    for (uint32_t i = 0; i < rx_parsed_packets.size(); i++)
    {
        delete rx_parsed_packets[i];
    }
}

void SerialManager::SetTxFlag()
{
    tx_is_free = true;
}


SerialManager::SerialStatus SerialManager::WriteSerialInterrupt(
    const Packet& packet,
    const uint32_t current_time)
{
    // Check if a transmission timed out
    if (TxWatchDog(current_time)) return SerialManager::ERROR;

    if (!tx_is_free) return SerialStatus::BUSY;

    tx_is_free = false;

    // Get the size of the packet + 3 because this includes the type, id, len
    tx_buffer_sz = static_cast<uint16_t>(packet.GetData(14, 10)) + 3;

    if (tx_buffer != nullptr) delete [] tx_buffer;
    tx_buffer = std::move(packet.ToBytes());

    HAL_UART_Transmit_IT(uart, tx_buffer, tx_buffer_sz);

    rx_watchdog_timeout = current_time + 10000;

    return SerialStatus::OK;
}

bool SerialManager::TxWatchDog(const uint32_t current_time)
{
    if (current_time <= tx_watchdog_timeout) return false;


    // TODO
    return false;
}

void SerialManager::RxEventTrigger(const uint32_t sz)
{
    has_rx_data = true;

    // Transfer to ring buff
    for (uint16_t i = 0; i < sz; i++)
    {
        rx_ring.Write(rx_buffer[i]);
    }
}

SerialManager::SerialStatus
SerialManager::ReadSerialInterrupt(const uint32_t current_time)
{
    if (!has_rx_data) return SerialStatus::EMPTY;

    // Read from the ring buff
    uint8_t data = 0;
    bool is_end = false;
    // Find the start
    while (data != 0xFF && !is_end)
    {
        // Read data into the data buffer
        rx_ring.Read(data, is_end);
    }

    // Couldn't find the start of the message
    if (is_end && data != 0xFF) return SerialStatus::ERROR;

    // Found the start, so create a packet
    rx_packet = new Packet();

    // Get the next 3 bytes and put them into the packet
    rx_packet->SetData(rx_ring.Read(), 0, 8);
    rx_packet->SetData(rx_ring.Read(), 8, 8);
    rx_packet->SetData(rx_ring.Read(), 16, 8);

    // Get the length of the incoming message
    uint16_t data_length = rx_packet->GetData(14, 10);

    // Move read n bytes from the ring to the packet
    for (uint16_t i = 0; i < data_length; i++)
    {
        rx_packet->SetData(rx_ring.Read(), 24 + i * 8, 8);
    }

    // Move to the vector of packets
    rx_parsed_packets.push_back(std::move(rx_packet));

    rx_packet = nullptr;

    StartRTIRx(256);

    return SerialManager::SerialStatus::OK;
}


// TODO notes
// Note - This is generally not used but is he for legacy purposes, will be
// optimized out by the compiler.
SerialManager::SerialStatus SerialManager::ReadSerialInterruptSpecialized(
    const uint32_t current_time)
{
    // Check if a transmission timed out
    if (RxWatchDog(current_time)) return SerialStatus::ERROR;

    // Check if data has come in
    if (!has_rx_data) return SerialStatus::EMPTY;

    if (rx_packet == nullptr)
    {
        // TODO should respond with an error/queue up an error message
        // via a flag
        if (rx_buffer[0] != 0xFF)
        {
            // TODO remove this magic number
            StartRx(Start_Bytes, current_time);
            return SerialManager::SerialStatus::EMPTY;
        }

        // Make a new packet
        rx_packet = new Packet();
        for (uint16_t i = 0; i < rx_buffer_sz-1; ++i)
        {
            rx_packet->SetData(rx_buffer[i+1], (i*8), 8);
        }

        uint8_t type = rx_packet->GetData(0, 6);
        uint8_t id = rx_packet->GetData(6, 8);
        uint16_t len = rx_packet->GetData(14, 10);

        // Check if this message is a UIMessage
        if (type != Packet::UIMessage)
        {
            StartRx(Start_Bytes, current_time);
            return SerialManager::SerialStatus::ERROR;
        }

        // If we get down to here we have a valid message incoming
        // Wait for the num of bytes specified by len
        StartRx(len, current_time);
        return SerialManager::SerialStatus::PARTIAL;
    }
    else
    {
        uint32_t bit_offset = rx_packet->BitsUsed();

        // Now we have received a full message move the data to the packet
        for (uint16_t i = 0; i < rx_buffer_sz; ++i)
        {
            rx_packet->SetData(rx_buffer[i], 24 + (i*8), 8);
        }

        // Move the packet onto the Vector
        bool was_pushed = rx_parsed_packets.push_back(std::move(rx_packet));

        // TODO if this is a message and we are behind, add to vector for
        // re-transmission
        if (!was_pushed) delete rx_packet;
        rx_packet = nullptr;
        StartRx(Start_Bytes, current_time);
        return SerialManager::SerialStatus::OK;
    }

    return SerialManager::SerialStatus::TIMEOUT;
}

bool SerialManager::RxWatchDog(const uint32_t current_time)
{
    // If there is still time then there is no problem
    if (current_time <= rx_watchdog_timeout) return false;

    // Reset the RX IT bits
    CLEAR_BIT(uart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
    CLEAR_BIT(uart->Instance->CR3, USART_CR3_EIE);

    // Reset the transfer
    uart->RxXferCount = 0U;

    // Restore the state
    uart->RxState = HAL_UART_STATE_READY;
    uart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    // Restore the IT bits
    SET_BIT(uart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
    SET_BIT(uart->Instance->CR3, USART_CR3_EIE);

    if (rx_packet != nullptr)
    {
        delete rx_packet;
        rx_packet = nullptr;
    }

    StartRx(Start_Bytes, current_time);
    return true;
}

Vector<Packet*>& SerialManager::GetPackets()
{
    return rx_parsed_packets;
}

// For some reason I don't get access to this so I just grabbed it.
HAL_StatusTypeDef UART_WaitOnFlagUntilTimeout(UART_HandleTypeDef *huart,
                                              uint32_t Flag,
                                              FlagStatus Status,
                                              uint32_t Tickstart,
                                              uint32_t Timeout)
{
  /* Wait until flag is set */
  while ((__HAL_UART_GET_FLAG(huart, Flag) ? SET : RESET) == Status)
  {
    /* Check for the Timeout */
    if (Timeout != HAL_MAX_DELAY)
    {
      if ((Timeout == 0U) || ((HAL_GetTick() - Tickstart) > Timeout))
      {
        /* Disable TXE, RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts for the interrupt process */
        ATOMIC_CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE | USART_CR1_TXEIE));
        ATOMIC_CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

        huart->gState  = HAL_UART_STATE_READY;
        huart->RxState = HAL_UART_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(huart);

        return HAL_TIMEOUT;
      }
    }
  }
  return HAL_OK;
}

SerialManager::SerialMessage SerialManager::ReadSerial(uint16_t max_size,
                                                       uint32_t timeout)
{
    constexpr unsigned int Byte_Size = 8;

    // TODO note- We should send a confirmation message.
    uint32_t read_start = 0U;
    SerialManager::SerialMessage msg;

    // Make sure the receive is ready
    if (uart->RxState != HAL_UART_STATE_READY)
    {
        msg.status = EMPTY;
        return msg;
    }
    if (max_size == 0U || timeout == 0U)
    {
        msg.status = ERROR;
        return msg;
    }

    /* Process Locked */
    if (uart->Lock == HAL_LOCKED)
    {
        msg.status = BUSY;
        return msg;
    }

    uart->Lock = HAL_LOCKED;

    uart->ErrorCode = HAL_UART_ERROR_NONE;
    uart->RxState = HAL_UART_STATE_BUSY_RX;
    uart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    // Get the time we started reading at
    read_start = HAL_GetTick();

    uart->RxXferSize = max_size;
    uart->RxXferCount = max_size;

    uart->Lock = HAL_UNLOCKED;

    uint8_t type = 0;
    uint8_t received_data = 0;
    unsigned int bit_offset = 0;
    bool start_received = false;

    do
    {
        if (UART_WaitOnFlagUntilTimeout(uart, UART_FLAG_RXNE, RESET,
            read_start, timeout) != HAL_OK)
        {
            // if (msg.packet.GetSize() > 0)
            //     msg.status = PARTIAL;
            // else
                msg.status = EMPTY;
            return msg;
        }

        if ((uart->Init.WordLength == UART_WORDLENGTH_9B) ||
            ((uart->Init.WordLength == UART_WORDLENGTH_8B)
            && (uart->Init.Parity == UART_PARITY_NONE)))
        {
            // Get the data from the uart instance data register 8 bit
            received_data = (uint8_t)(uart->Instance->DR & (uint8_t)0x00FF);
        }
        else
        {
            // Get the data from the uart instance data register 7 bit
            received_data = (uint8_t)(uart->Instance->DR & (uint8_t)0x007F);
        }


        if (received_data == 0xFF && !start_received)
        {
            start_received = true;
            continue;
        }

        if (received_data != 0xFF && !start_received)
        {
            continue;
        }

        msg.packet.SetData(received_data, bit_offset, Byte_Size);
        bit_offset += Byte_Size;

        // After the first two bytes we will have the type and size
        if (bit_offset == 16)
        {
            // Get the type
            unsigned char type = msg.packet.GetData(0, 6);

            // If this is a network debug just ignore it
            if (type == Packet::PacketTypes::NetworkDebug)
            {
                uart->RxXferCount = 0U;
                uart->RxState = HAL_UART_STATE_READY;
                msg.status = EMPTY;

                return msg;
            }

            // Set the expected transfer count and go to the next iteration
            uart->RxXferCount = msg.packet.GetData(6, 10);
            continue;
        }

        // Decrement the expected bytes
        --uart->RxXferCount;
    } while (uart->RxXferCount > 0U);

    // Set the state back to ready
    uart->RxXferCount = 0U;
    uart->RxState = HAL_UART_STATE_READY;
    msg.status = OK;
    return msg;
}

SerialManager::SerialStatus SerialManager::WriteSerial(const Packet& packet,
                                                       const uint32_t timeout)
{
    // TODO Should take a packet and send the data that way instead.



    // TODO-note we should wait for a confirmation message
    // TODO-send the size of the message as the first byte..
    uint32_t send_start = 0U;

    // Check that there isn't already a transmit
    if (uart->gState != HAL_UART_STATE_READY) return EMPTY;
    if (uart->Lock == HAL_LOCKED) return BUSY;

    // Get the size +2 for the first two bytes
    unsigned int data_size = packet.GetData(6, 10)+2;

    if (data_size == 0) return ERROR;

    uart->Lock = HAL_LOCKED;

    uart->ErrorCode = HAL_UART_ERROR_NONE;
    uart->gState = HAL_UART_STATE_BUSY_TX;

    send_start = HAL_GetTick();

    uart->TxXferSize = static_cast<uint16_t>(data_size);
    uart->TxXferCount = static_cast<uint16_t>(data_size);

    uart->Lock = HAL_UNLOCKED;

    uint32_t idx = 0;
    unsigned char data = 0;
    unsigned int offset = 0;
    while (uart->TxXferCount > 0U)
    {
        if (UART_WaitOnFlagUntilTimeout(uart, UART_FLAG_TXE, RESET, send_start,
            timeout) != HAL_OK)
        {
            if (uart->TxXferCount < uart->TxXferSize)
                return PARTIAL;
            return TIMEOUT;
        }

        uart->Instance->DR = static_cast<uint8_t>(packet.GetData(offset, 8));
        offset += 8;
        uart->TxXferCount--;
    }


    if (UART_WaitOnFlagUntilTimeout(uart, UART_FLAG_TC, RESET, send_start,
        timeout) != HAL_OK)
    {
      return TIMEOUT;
    }

    uart->gState = HAL_UART_STATE_READY;

    return OK;
}


/* Private functions */
void SerialManager::StartRx(const uint16_t num_bytes,
                            const uint32_t current_time)
{
    // Delete the current rx_buffer
    if (rx_buffer) delete [] rx_buffer;

    // Call the data register and ignore the result to get the hardware to
    // reset itself
    volatile uint32_t dr = uart->Instance->DR;

    // Standard packet size
    rx_buffer_sz = num_bytes;

    // Create a new buffer
    rx_buffer = new uint8_t[rx_buffer_sz];

    // Set rx time
    rx_watchdog_timeout = current_time + 10000;

    // Reset the rx flag
    has_rx_data = false;

    // Begin the receive IT
    HAL_UART_Receive_IT(uart, rx_buffer, rx_buffer_sz);
}


// TODO do I need to pass the num_bytes?
// Maybe should do a check to make sure only one type of rx interrupt is used?
void SerialManager::StartRTIRx(const uint16_t num_bytes)
{
    // Delete the current rx_buffer
    if (rx_buffer) delete [] rx_buffer;

    // Standard packet size
    rx_buffer_sz = num_bytes;

    // Create a new buffer
    rx_buffer = new uint8_t[rx_buffer_sz];

    // Reset the rx flag
    has_rx_data = false;

    // Begin the receive IT
    HAL_UARTEx_ReceiveToIdle_IT(uart, rx_buffer, rx_buffer_sz);
}
