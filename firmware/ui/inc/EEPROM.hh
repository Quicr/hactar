#pragma once

#include "PortPin.hh"
#include "Vector.hh"

class EEPROM
{
public:
    EEPROM(I2C_HandleTypeDef& hi2c, const unsigned short reserved, const unsigned short max_sz, const unsigned char operation_delay=5) :
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
    const unsigned short Write(T* data, const unsigned int sz=1)
    {
        const unsigned short addr = next_address;
        const size_t data_size = sizeof(*data) * sz;
        unsigned char* to_write = (unsigned char*)(void*)data;

        // If the write failed return a -1 address
        if (PerformWrite(to_write, addr, data_size) != HAL_OK)
            return -1;

        // +1 for address
        next_address += data_size + 1;
        return addr;
    }

    template<typename T>
    const int Write(T data, const unsigned int sz=1)
    {
        const unsigned short addr = next_address;
        const size_t data_size = sizeof(data) * sz;
        unsigned char* to_write = (unsigned char*)(void*)&data;

        // If the write failed return a -1 address
        if (PerformWrite(to_write, addr, data_size) != HAL_OK)
            return -1;

        // +1 for address
        next_address += data_size + 1;
        return addr;
    }

    template<typename T>
    const HAL_StatusTypeDef Write(const unsigned int address,
                                  T& data,
                                  const unsigned int sz=1)
    {
        const size_t data_size = sizeof(data) * sz;
        unsigned char* to_write = (unsigned char*)(void*)&data;

        return PerformWrite(to_write, address, data_size);
    }

    const HAL_StatusTypeDef WriteByte(const unsigned int address,
                                      unsigned char data)
    {
        unsigned char write_byte[2];
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
        unsigned char set_address[1] = { address };
        HAL_StatusTypeDef set_address_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, set_address, 1, HAL_MAX_DELAY);

        if (set_address_res != HAL_OK) return set_address_res;

        HAL_Delay(operation_delay);

        unsigned short output_sz = sizeof(*data) * sz;

        // Create a new pointer to this data
        unsigned char* output_data = new unsigned char[sz];
        HAL_StatusTypeDef read_res = HAL_I2C_Master_Receive(i2c,
            Read_Condition, output_data, output_sz, HAL_MAX_DELAY);
        HAL_Delay(operation_delay);

        // Cast it back to normal data for T
        *data = (T*)output_data;

        return read_res;
    }

    unsigned char ReadByte(const uint8_t address)
    {
        // Set the address to read from
        uint8_t set_address[1] = { address };
        HAL_StatusTypeDef set_address_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, set_address, 1, HAL_MAX_DELAY);

        if (set_address_res != HAL_OK) return set_address_res;

        // Read data
        unsigned char data[1] = {0};
        HAL_StatusTypeDef read_res = HAL_I2C_Master_Receive(i2c, Read_Condition,
            data, 1, HAL_MAX_DELAY);
        HAL_Delay(operation_delay);

        if (read_res != HAL_OK)
            return (unsigned char)255; // Err

        return data[0];
    }

    void Clear()
    {
        const unsigned int Bytes_Sz = 256;
        const unsigned int Bytes_W_Address_Sz = Bytes_Sz + 1;
        // Reset the write address to the start
        next_address = 0;

        // Number of bytes we have to delete
        unsigned int bytes_loop = max_sz / Bytes_Sz;

        // Write a bunch of 1s
        unsigned char clear_bytes[Bytes_W_Address_Sz] = { 0 };
        for (unsigned int i = 1; i < Bytes_W_Address_Sz; i++)
        {
            clear_bytes[i] = 0xFF;
        }

        while (bytes_loop--)
        {
            clear_bytes[0] = next_address;

            HAL_I2C_Master_Transmit(i2c, Write_Condition, clear_bytes, Bytes_Sz,
                HAL_MAX_DELAY);

            next_address += Bytes_Sz;

            HAL_Delay(operation_delay);
        }

        // Reset the next address back to the start after the reserved space
        next_address = reserved;
    }

    const unsigned int Size() const
    {
        return max_sz;
    }

    const unsigned int NextAddress() const
    {
        return next_address;
    }

    const float Usage() const
    {
        return next_address / max_sz;
    }

private:
    HAL_StatusTypeDef PerformWrite(unsigned char* data,
                             const unsigned short address,
                             const unsigned short data_size)
    {
        const size_t data_w_address = data_size + 1;

        // Copy to_write to a new array
        unsigned char write_bytes[data_w_address];

        write_bytes[0] = address;
        write_bytes[1] = data_size;
        // Write the length
        HAL_StatusTypeDef len_write_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, write_bytes, 2, HAL_MAX_DELAY);
        HAL_Delay(operation_delay);

        if (len_write_res != HAL_OK)
            return len_write_res;

        // TODO rewrite hal transmit so I can send an open message that has
        // just the address and then continue the message with the rest
        // of the data, instead of copying it. so slow.
        write_bytes[0] = address + 1;
        for (size_t i = 0; i < data_size; ++i)
        {
            write_bytes[i+1] = data[i];
        }

        // Write data
        HAL_StatusTypeDef write_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, write_bytes, data_w_address, HAL_MAX_DELAY);

        HAL_Delay(operation_delay);

        return write_res;
    }

    static constexpr unsigned char Read_Condition     = 0xA1;
    static constexpr unsigned char Write_Condition    = 0xA0;

    I2C_HandleTypeDef* i2c;
    const unsigned short reserved;
    const unsigned short max_sz;
    unsigned short next_address;
    unsigned char operation_delay;
};