#pragma once

#include "stm32.h"

template <const size_t Size_In_Bytes>
class M24C02_EEPROM
{
public:
    M24C02_EEPROM(I2C_HandleTypeDef& hi2c,
                  const uint16_t reserved_area_size,
                  const uint8_t write_operation_timeout_ms = 5) :
        i2c(&hi2c),
        reserved_area_size(reserved_area_size),
        next_address(reserved_area_size),
        write_operation_timeout_ms(write_operation_timeout_ms)
    {
    }

    ~M24C02_EEPROM()
    {
        i2c = nullptr;
    }

    /* M24C02_EEPROM::Write
     * Description - This function takes any data form and turns it into bytes
     * and writes it to the eeprom at the next available address
     */
    template <typename T>
    int16_t Write(T* data, const uint32_t sz)
    {
        const uint8_t addr = next_address;
        const size_t data_size = sizeof(T) * sz;

        // If the write failed return a -1 address
        if (PerformWrite((uint8_t*)(void*)data, addr, data_size) != HAL_OK)
        {
            return -1;
        }

        // +1 for address
        next_address += data_size + 1;
        return static_cast<int16_t>(addr);
    }

    template <typename T>
    int16_t Write(T data, const uint32_t sz)
    {
        const uint16_t addr = next_address;
        const size_t data_size = sizeof(T) * sz;

        // If the write failed return a -1 address
        if (PerformWrite((uint8_t*)(void*)&data, addr, data_size) != HAL_OK)
        {
            return -1;
        }

        // +1 for address
        next_address += data_size + 1;
        return static_cast<int16_t>(addr);
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

        HAL_StatusTypeDef write_byte_res =
            HAL_I2C_Master_Transmit(i2c, SLA_W, write_byte, sizeof(write_byte), HAL_MAX_DELAY);
        HAL_Delay(write_operation_timeout_ms);

        return write_byte_res;
    }

    /**
     * This function assumes you know what you want.
     */
    template <typename T>
    HAL_StatusTypeDef Read(const uint8_t address, T* data, const uint16_t sz = 1)
    {
        if (address >= Size_In_Bytes)
        {
            return HAL_ERROR;
        }

        // Set the address to read from
        HAL_StatusTypeDef address_res =
            HAL_I2C_Master_Transmit(i2c, SLA_W, (uint8_t*)&address, 1, HAL_MAX_DELAY);

        if (address_res != HAL_OK)
        {
            return address_res;
        }

        const uint16_t output_sz = sizeof(char) * sz;

        HAL_StatusTypeDef read_res =
            HAL_I2C_Master_Receive(i2c, SLA_R, (uint8_t*)data, output_sz, HAL_MAX_DELAY);

        return read_res;
    }

    int16_t ReadByte(const uint8_t address)
    {
        if (address >= Size_In_Bytes)
        {
            return -1;
        }

        // Set the address in the eeprom we will be reading the data from
        if (HAL_OK != HAL_I2C_Master_Transmit(i2c, SLA_W, (uint8_t*)&address, 1, HAL_MAX_DELAY))
        {
            return -1;
        }

        // Read data
        uint8_t data;
        HAL_StatusTypeDef read_res = HAL_I2C_Master_Receive(i2c, SLA_R, &data, 1, HAL_MAX_DELAY);

        if (read_res != HAL_OK)
        {
            return -1;
        }

        return data;
    }

    bool Fill(uint8_t fill_value = 0xFF)
    {
        // Reset the write address to the start
        next_address = 0;

        // Address to start writing is 0
        uint8_t fill_byte[Size_In_Bytes + 1] = {0};

        // Write a bunch of 1s
        HAL_StatusTypeDef clear_res = HAL_ERROR;
        for (int16_t i = 0; i < Size_In_Bytes; i++)
        {
            fill_byte[i + 1] = fill_value;
        }

        // Write to the eeprom starting from address zero, and filling all bytes with the fill_value
        clear_res =
            HAL_I2C_Master_Transmit(i2c, SLA_W, fill_byte, Size_In_Bytes + 1, HAL_MAX_DELAY);

        // Give the eeprom time to write
        HAL_Delay(write_operation_timeout_ms * Size_In_Bytes);

        // Reset the next address back to the start after the reserved_area_size space
        next_address = reserved_area_size;

        return clear_res == HAL_OK;
    }

    uint16_t Size() const
    {
        return Size_In_Bytes;
    }

    uint16_t NextAddress() const
    {
        return next_address;
    }

    float Usage() const
    {
        return next_address / Size_In_Bytes;
    }

private:
    HAL_StatusTypeDef PerformWrite(uint8_t* data, const uint8_t address, const uint16_t data_size)
    {
        if (data_size > Size_In_Bytes - reserved_area_size)
        {
            return HAL_ERROR;
        }

        // Copy to_write to a new array
        uint8_t write_bytes[2];

        // Write the length
        write_bytes[0] = address;
        write_bytes[1] = data_size;
        HAL_StatusTypeDef len_write_res =
            HAL_I2C_Master_Transmit(i2c, SLA_W, write_bytes, 2, HAL_MAX_DELAY);
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
            write_res = HAL_I2C_Master_Transmit(i2c, SLA_W, write_bytes, 2, HAL_MAX_DELAY);

            if (write_res != HAL_OK)
            {
                return write_res;
            }

            HAL_Delay(write_operation_timeout_ms);
        }

        return write_res;
    }

    static constexpr uint8_t SLA_R = 0xA1;
    static constexpr uint8_t SLA_W = 0xA0;

    I2C_HandleTypeDef* i2c;
    const uint16_t reserved_area_size;
    int16_t next_address;
    uint8_t write_operation_timeout_ms;
};