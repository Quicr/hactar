#pragma once

#include "stm32.h"
#include <span>

class SPIDevice
{
public:
    SPIDevice(SPI_HandleTypeDef* hspi, GPIO_TypeDef* cs_port, const uint16_t cs_pin) :
        spi(hspi),
        cs_port(cs_port),
        cs_pin(cs_pin),
        transmitting(false)
    {
    }

    void Transmit()
    {
        // TODO checking for wanting to transmit to multiple devices
        Select();

        std::span<const uint8_t> data = UpdateTransmit();

        // HAL_SPI_Transmit_DMA(&spi, data.data(), data.)
    }

    void Transmit(const uint8_t* data, const size_t len)
    {
        Select();
        HAL_SPI_Transmit(spi, data, len, HAL_MAX_DELAY);
        Deselect();
    }

    void TransmitInterrupt()
    {

        // If we are done transmitting deselect
    }

    void Receive()
    {
        // TODO checking for wanting to receive from multiple devices
    }

    void ReceiveInterrupt()
    {
    }

    void Select()
    {
        HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
    }

    void Deselect()
    {
        HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    }

    virtual std::span<const uint8_t> UpdateTransmit() = 0;
    virtual std::span<const uint8_t> UpdateReceive() = 0;

protected:
    SPI_HandleTypeDef* spi;
    GPIO_TypeDef* cs_port;
    const uint16_t cs_pin;
    bool transmitting;
};