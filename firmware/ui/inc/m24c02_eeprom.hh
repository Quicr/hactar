#pragma once

#include "logger.hh"
#include "stm32.h"
#include "stm32f4xx_hal_def.h"
#include <algorithm>
#include <cstring>

class M24C02_EEPROM
{
public:
    M24C02_EEPROM(I2C_HandleTypeDef& hi2c,
                  const size_t size_in_bytes,
                  const uint8_t write_operation_timeout_ms = 10) :
        i2c(&hi2c),
        size_in_bytes(size_in_bytes),
        write_operation_timeout_ms(write_operation_timeout_ms)
    {
    }

    ~M24C02_EEPROM()
    {
        i2c = nullptr;
    }

    template <typename T>
    HAL_StatusTypeDef Write(const uint8_t address, T& data, const uint32_t sz)
    {
        const size_t data_size = sizeof(T) * sz;

        return PerformWrite((uint8_t*)(void*)&data, address, data_size);
    }

    template <typename T>
    HAL_StatusTypeDef Write(const uint8_t address, T* data, const uint32_t sz)
    {
        const size_t data_size = sizeof(T) * sz;

        return PerformWrite((uint8_t*)(void*)&*data, address, data_size);
    }

    HAL_StatusTypeDef WriteByte(const uint8_t address, uint8_t data)
    {
        uint8_t write_byte[2]{address, data};

        HAL_StatusTypeDef write_byte_res = HAL_I2C_Master_Transmit(
            i2c, Device_Addr, write_byte, sizeof(write_byte), HAL_MAX_DELAY);
        HAL_Delay(write_operation_timeout_ms);

        return write_byte_res;
    }

    /**
     * This function assumes you know what you want.
     */
    template <typename T>
    HAL_StatusTypeDef Read(const uint8_t address, T* data, const uint16_t sz = 1) const
    {
        if (address >= size_in_bytes)
        {
            return HAL_ERROR;
        }

        // Set the address to read from
        HAL_StatusTypeDef address_res =
            HAL_I2C_Master_Transmit(i2c, Device_Addr, (uint8_t*)&address, 1, HAL_MAX_DELAY);

        if (address_res != HAL_OK)
        {
            return address_res;
        }

        const uint16_t output_sz = sizeof(char) * sz;

        HAL_StatusTypeDef read_res =
            HAL_I2C_Master_Receive(i2c, Device_Addr, (uint8_t*)data, output_sz, HAL_MAX_DELAY);

        return read_res;
    }

    int16_t ReadByte(const uint8_t address) const
    {
        if (address >= size_in_bytes)
        {
            return -1;
        }

        // Set the address in the eeprom we will be reading the data from
        if (HAL_OK
            != HAL_I2C_Master_Transmit(i2c, Device_Addr, (uint8_t*)&address, 1, HAL_MAX_DELAY))
        {
            return -1;
        }

        // Read data
        uint8_t data = 0;
        HAL_StatusTypeDef read_res =
            HAL_I2C_Master_Receive(i2c, Device_Addr, &data, 1, HAL_MAX_DELAY);

        if (read_res != HAL_OK)
        {
            return -1;
        }

        return data;
    }

    // For reasons unknown to me, this will cause some maaaaaaaaaaaaaaaaaaaaaassive issues
    // the chip gets locked up and then it causes all kinds of havoc with the ui chip.
    // Probably related to the i2c being all bonkered from it.
    // but I need to figure out how to fix it without
    // too many bricks.
    bool Fill(uint8_t fill_value = 0xFF)
    {

        HAL_StatusTypeDef clear_res;
        uint8_t data[Page_Size]{0};

        for (int i = 0; i < Page_Size; ++i)
        {
            data[i] = fill_value;
        }

        const uint16_t num_pages = size_in_bytes / Page_Size;
        for (uint16_t i = 0; i < num_pages; ++i)
        {
            clear_res = PerformWrite(data, i * Page_Size, Page_Size);

            if (clear_res != HAL_OK)
            {
                return false;
            }
        }

        return clear_res == HAL_OK;
    }

    uint16_t Size() const
    {
        return size_in_bytes;
    }

private:
    HAL_StatusTypeDef PerformWrite(uint8_t* data, const uint8_t address, uint16_t data_size)
    {
        if (address + data_size > size_in_bytes)
        {
            UI_LOG_ERROR("address + data_size is too large for EEPROM");
            return HAL_ERROR;
        }

        // Find the boundaries on the data starting address and where it ends
        uint8_t next_addr = address;
        uint8_t page[Page_Size + 1];
        HAL_StatusTypeDef res = HAL_OK;
        const uint16_t num_pages =
            std::min(1, (data_size + (address % Page_Size) + Page_Size - 1) / Page_Size);

        for (uint16_t i = 0; i < num_pages; ++i)
        {
            page[0] = next_addr;

            const uint16_t len = std::min(data_size, Page_Size);
            std::memcpy(page + 1, data + i * Page_Size, len);

            res = HAL_I2C_Master_Transmit(i2c, Device_Addr, page, sizeof(page), HAL_MAX_DELAY);

            if (res != HAL_OK)
            {
                UI_LOG_ERROR("Failed to write to eeprom");
                return res;
            }

            data_size -= len;

            HAL_Delay(write_operation_timeout_ms);
        }

        return res;
    }

    // Shifted pre left 1 bit
    static constexpr uint8_t Device_Addr = 0x50 << 1;
    static constexpr uint16_t Page_Size = 16;

    I2C_HandleTypeDef* i2c;
    const size_t size_in_bytes;
    const uint8_t write_operation_timeout_ms;
};
