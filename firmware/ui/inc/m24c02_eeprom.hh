#pragma once

#include "stm32.h"

class M24C02_EEPROM
{
public:
    M24C02_EEPROM(I2C_HandleTypeDef& hi2c,
                  const size_t size_in_bytes,
                  const uint8_t write_operation_timeout_ms = 8) :
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

    bool Fill(uint8_t fill_value = 0xFF)
    {
        // Address to start writing is 0
        uint8_t fill_byte[size_in_bytes + 1] = {0};

        // Write a bunch of 1s
        HAL_StatusTypeDef clear_res = HAL_ERROR;
        for (int16_t i = 0; i < size_in_bytes; i++)
        {
            fill_byte[i + 1] = fill_value;
        }

        // Write to the eeprom starting from address zero, and filling all bytes with the fill_value
        clear_res =
            HAL_I2C_Master_Transmit(i2c, Device_Addr, fill_byte, size_in_bytes + 1, HAL_MAX_DELAY);

        // Give the eeprom time to write
        HAL_Delay(write_operation_timeout_ms * size_in_bytes);

        return clear_res == HAL_OK;
    }

    uint16_t Size() const
    {
        return size_in_bytes;
    }

private:
    HAL_StatusTypeDef PerformWrite(uint8_t* data, const uint8_t address, const uint16_t data_size)
    {
        if (data_size > size_in_bytes)
        {
            return HAL_ERROR;
        }

        // Copy to_write to a new array
        uint8_t write_bytes[2];

        // Write the length
        write_bytes[0] = address;
        write_bytes[1] = data_size;
        HAL_StatusTypeDef len_write_res =
            HAL_I2C_Master_Transmit(i2c, Device_Addr, write_bytes, 2, HAL_MAX_DELAY);
        HAL_Delay(write_operation_timeout_ms);

        if (len_write_res != HAL_OK)
        {
            return len_write_res;
        }

        // Write data
        HAL_StatusTypeDef write_res = HAL_ERROR;
        uint8_t data_addr = address + 1;
        for (int16_t i = 0; i < data_size; ++i)
        {
            write_bytes[0] = address + 1 + i;
            write_bytes[1] = data[i];
            write_res = HAL_I2C_Master_Transmit(i2c, Device_Addr, write_bytes, 2, HAL_MAX_DELAY);

            if (write_res != HAL_OK)
            {
                return write_res;
            }

            HAL_Delay(write_operation_timeout_ms);
        }

        return write_res;
    }

    // Shifted pre left 1 bit
    static constexpr uint8_t Device_Addr = 0x50 << 1;

    I2C_HandleTypeDef* i2c;
    const size_t size_in_bytes;
    uint8_t write_operation_timeout_ms;
};