#pragma once

#include "PortPin.hh"
#include "Vector.hh"

class EEPROM
{
public:
    EEPROM(I2C_HandleTypeDef& hi2c, const unsigned short reserved, const unsigned short max_sz) :
        i2c(&hi2c),
        reserved(reserved),
        max_sz(max_sz),
        next_address(reserved),
        read_value(nullptr)
    {

    }

    ~EEPROM()
    {
        i2c = nullptr;
        if (read_value) delete read_value;
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

        PerformWrite(to_write, addr, data_size);

        // +1 for address
        next_address += data_size + 1;
        return addr;
    }

    template<typename T>
    const unsigned short Write(T data, const unsigned int sz=1)
    {
        const unsigned short addr = next_address;
        const size_t data_size = sizeof(data) * sz;
        unsigned char* to_write = (unsigned char*)(void*)&data;

        PerformWrite(to_write, addr, data_size);

        // +1 for address
        next_address += data_size + 1;
        return addr;
    }

    template<typename T>
    const void Write(const unsigned int address,
                     T& data,
                     const unsigned int sz=1)
    {
        const size_t data_size = sizeof(data) * sz;
        const size_t data_w_address = data_size + 1;
        unsigned char* to_write = (unsigned char*)(void*)&data;

        PerformWrite(to_write, address, data_size);
    }

    // Assumes the user knows what they want
    template<typename T>
    void Read(const uint8_t address, T& data, const uint16_t sz=1)
    {
        // Set the address to read from
        unsigned char set_address[1] = { address };
        HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(i2c, Write_Condition, set_address, 1,
            HAL_MAX_DELAY);

        unsigned short output_sz = sizeof(data) * sz;
        unsigned char* output_data = (unsigned char*)(void*)&data;
        res = HAL_I2C_Master_Receive(i2c, Read_Condition, output_data, output_sz,
            HAL_MAX_DELAY);

        // Cast it back to normal data for T
        data = *(T*)output_data;
    }

    // TODO probably should be const
    unsigned char* Read(const uint8_t address)
    {
        // Set the address to read from
        unsigned char set_address[1] = { address };
        HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(i2c, Write_Condition, set_address, 1,
            HAL_MAX_DELAY);

        // Get the length of the data
        unsigned char length;
        res = HAL_I2C_Master_Receive(i2c, Read_Condition, &length, 1, HAL_MAX_DELAY);

        // If we have a read value, delete it
        if (read_value) delete read_value;
        read_value = new unsigned char[length];
        res = HAL_I2C_Master_Receive(i2c, Read_Condition, read_value, length,
            HAL_MAX_DELAY);

        return read_value;
    }

    unsigned char ReadByte(const uint8_t address)
    {
        // Set the address to read from
        uint8_t set_address[1] = { address };
        HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(i2c, Write_Condition, set_address, 1,
            HAL_MAX_DELAY);

        // Read data
        unsigned char data[1] = {0};
        res = HAL_I2C_Master_Receive(i2c, Read_Condition, data, 1, HAL_MAX_DELAY);
        return data[0];
    }

    void Clear()
    {
        // Reset the write address to the start
        next_address = 0;

        // Number of bytes we have to delete
        unsigned int bytes_loop = max_sz / 256;

        // Write a bunch of 1s
        unsigned char clear_bytes[256] = { 0 };
        for (int i = 0; i < 256; i++)
        {
            clear_bytes[i] = 0xFF;
        }

        while (bytes_loop--)
        {
            // TODO manual
        }

        // Reset the next address back to the start
        next_address = 0;
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
    void PerformWrite(unsigned char* data,
                             const unsigned short address,
                             const unsigned short data_size)
    {
        const size_t data_w_address = data_size + 1;

        // Copy to_write to a new array
        unsigned char bytes[data_w_address];

        bytes[0] = address;
        bytes[1] = data_size;
        // Write the length
        HAL_StatusTypeDef write_res = HAL_I2C_Master_Transmit(i2c, Write_Condition, bytes, 2,
            HAL_MAX_DELAY);


        // TODO rewrite hal transmit so I can send an open message that has
        // just the address and then continue the message with the rest
        // of the data, instead of copying it. so slow.
        bytes[0] = address + 1;
        for (size_t i = 0; i < data_size; ++i)
        {
            bytes[i+1] = data[i];
        }

        // Write data
        write_res = HAL_I2C_Master_Transmit(i2c, Write_Condition, bytes, data_w_address,
            HAL_MAX_DELAY);

        return;
    }

    static constexpr unsigned char Read_Condition     = 0xA1;
    static constexpr unsigned char Write_Condition    = 0xA0;

    I2C_HandleTypeDef* i2c;
    const unsigned short reserved;
    const unsigned short max_sz;
    unsigned short next_address;
    unsigned char* read_value;
};