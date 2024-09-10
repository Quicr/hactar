#pragma once

#include "port_pin.hh"
#include <vector>

class EEPROM
{
public:
    EEPROM(I2C_HandleTypeDef& hi2c, const uint16_t reserved, const uint16_t max_sz, const uint8_t operation_delay=5) :
        i2c(&hi2c),
        reserved(reserved),
        max_sz(max_sz),
        next_address(reserved),
        operation_delay(operation_delay)
    {

    }

    ~EEPROM()
    {
        i2c = nullptr;
    }

    // TODO we need to make sure that write doesn't take too large of a data
    // size and overwrite some data


    /* EEPROM::Write
     * Description - This function takes any data form and turns it into bytes
     * and writes it to the eeprom at the next available address
    */
    template<typename T>
    uint16_t Write(T* data, const int32_t sz=1)
    {
        const uint16_t addr = next_address;
        const size_t data_size = sizeof(*data) * sz;
        uint8_t* to_write = (uint8_t*)(void*)data;

        // If the write failed return a -1 address
        if (PerformWrite(to_write, addr, data_size) != HAL_OK)
            return -1;

        // +1 for address
        next_address += data_size + 1;
        return addr;
    }

    template<typename T>
    uint16_t Write(T data, const int32_t sz=1)
    {
        const uint16_t addr = next_address;
        const size_t data_size = sizeof(T) * sz;
        uint8_t* to_write = (uint8_t*)(void*)&data;

        // If the write failed return a -1 address
        if (PerformWrite(to_write, addr, data_size) != HAL_OK)
            return -1;

        // +1 for address
        next_address += data_size + 1;
        return addr;
    }

    template<typename T>
    HAL_StatusTypeDef Write(const int32_t address, T& data, const int32_t sz=1)
    {
        const size_t data_size = sizeof(T) * sz;
        uint8_t* to_write = (uint8_t*)(void*)&data;

        return PerformWrite(to_write, address, data_size);
    }

    template<typename T>
    HAL_StatusTypeDef Write(const int32_t address, T* data, const int32_t sz=1)
    {
        const size_t data_size = sizeof(T) * sz;
        uint8_t* to_write = (uint8_t*)(void*)&*data;

        return PerformWrite(to_write, address, data_size);
    }

    HAL_StatusTypeDef WriteByte(const int32_t address,
                                      uint8_t data)
    {
        uint8_t write_byte[2];
        write_byte[0] = address;
        write_byte[1] = data;

        HAL_StatusTypeDef write_byte_res = HAL_I2C_Master_Transmit(
            i2c, Write_Condition, write_byte, 2, HAL_MAX_DELAY);
        HAL_Delay(operation_delay);

        return write_byte_res;
    }

    // Assumes the user knows what they want
    template<typename T>
    HAL_StatusTypeDef Read(const uint8_t address, T** data, const uint16_t sz=1)
    {
        // Set the address to read from
        uint8_t set_address[1] = { address };
        HAL_StatusTypeDef set_address_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, set_address, 1, HAL_MAX_DELAY);

        if (set_address_res != HAL_OK) return set_address_res;

        HAL_Delay(operation_delay);

        // sizeof(**data) is not needed??
        uint16_t output_sz = sizeof(**data) * sz;
        // uint16_t output_sz = sz;

        // Create a new pointer to this data
        uint8_t* output_data = new uint8_t[output_sz];
        HAL_StatusTypeDef read_res = HAL_I2C_Master_Receive(i2c,
            Read_Condition, output_data, output_sz, HAL_MAX_DELAY);
        HAL_Delay(operation_delay);

        // Cast it back to normal data for T
        *data = (T*)output_data;

        return read_res;
    }

    int16_t ReadByte(const uint8_t address)
    {
        // Set the address to read from
        uint8_t set_address[1] = { address };
        HAL_StatusTypeDef set_address_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, set_address, 1, HAL_MAX_DELAY);

        if (set_address_res != HAL_OK) return (int16_t)-1;

        // Read data
        uint8_t data[1] = {0};
        HAL_StatusTypeDef read_res = HAL_I2C_Master_Receive(i2c, Read_Condition,
            data, 1, HAL_MAX_DELAY);
        HAL_Delay(operation_delay);

        if (read_res != HAL_OK)
            return (int16_t)-1; // Err

        return data[0];
    }

    bool Clear()
    {
        // Reset the write address to the start
        next_address = 0;

        uint8_t clear_byte[2];
        clear_byte[1] = 0xFF;

        // Write a bunch of 1s
        HAL_StatusTypeDef clear_res = HAL_ERROR;
        for (int16_t i = 0; i < max_sz; i++)
        {
            clear_byte[0] = next_address++;

            clear_res = HAL_I2C_Master_Transmit(i2c, Write_Condition,
                clear_byte, 2, HAL_MAX_DELAY);

            HAL_Delay(operation_delay);
        }

        // Reset the next address back to the start after the reserved space
        next_address = reserved;

        return clear_res == HAL_OK;
    }

    uint16_t Size() const
    {
        return max_sz;
    }

    uint16_t NextAddress() const
    {
        return next_address;
    }

    float Usage() const
    {
        return next_address / max_sz;
    }

private:
    HAL_StatusTypeDef PerformWrite(uint8_t* data,
                             const uint16_t address,
                             const uint16_t data_size)
    {
        // Copy to_write to a new array
        uint8_t write_bytes[2];

        // Write the length
        write_bytes[0] = address;
        write_bytes[1] = data_size;
        HAL_StatusTypeDef len_write_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, write_bytes, 2, HAL_MAX_DELAY);
        HAL_Delay(operation_delay);

        if (len_write_res != HAL_OK)
            return len_write_res;

        // Write data
        HAL_StatusTypeDef write_res = HAL_ERROR;
        for (uint16_t i = 0; i < data_size; ++i)
        {
            write_bytes[0] = (address+1) + i;
            write_bytes[1] = data[i];
            write_res = HAL_I2C_Master_Transmit(i2c,
                Write_Condition, write_bytes, 2, HAL_MAX_DELAY);

            if (write_res != HAL_OK)
                return write_res;

            HAL_Delay(operation_delay);
        }

        return write_res;
    }

    static constexpr uint8_t Read_Condition     = 0xA1;
    static constexpr uint8_t Write_Condition    = 0xA0;

    I2C_HandleTypeDef* i2c;
    const uint16_t reserved;
    const uint16_t max_sz;
    uint16_t next_address;
    uint8_t operation_delay;
};