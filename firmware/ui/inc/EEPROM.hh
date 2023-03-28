#pragma once

#include "PortPin.hh"
#include "Vector.hh"

class EEPROM
{
public:
    EEPROM(I2C_HandleTypeDef& hi2c, const unsigned int max_sz) :
        i2c(&hi2c),
        max_sz(max_sz),
        next_address(0)
    {

    }

    ~EEPROM()
    {
        i2c = nullptr;
    }


    /* EEPROM::Write
     * Description - This function takes any data form and turns it into bytes
     * and writes it to the eeprom at the next available address
    */
    template<typename T>
    const unsigned int Write(T* data, const unsigned int sz=1)
    {
        const unsigned short addr = next_address;
        const size_t data_size = sizeof(*data) * sz;
        unsigned char* to_write = (unsigned char*)(void*)&data;

        PerformWrite(to_write, addr, data_size);

        next_address += data_size;
        return addr;
    }

    // template<typename T>
    // const unsigned int Write(T data, const unsigned int sz=1)
    // {
    //     const unsigned short addr = next_address;
    //     size_t x = sizeof(unsigned char);
    //     const size_t data_size = sizeof(data) * sz;
    //     unsigned char* to_write = (unsigned char*)(void*)&data;

    //     PerformWrite(to_write, addr, data_size);

    //     next_address += data_size;
    //     return addr;
    // }

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

    template<typename T>
    void Read(const uint8_t address, T& data, const uint16_t sz=1)
    {
        // Set the address to read from
        uint8_t set_address[1] = { address };
        HAL_I2C_Master_Transmit(i2c, Write_Condition, set_address, 1,
            HAL_MAX_DELAY);

        // Read data
        unsigned short output_sz = sizeof(data) * sz;
        unsigned char* output_data = (unsigned char*)(void*)&data;
        HAL_I2C_Master_Receive(i2c, Read_Condition, output_data, output_sz,
            HAL_MAX_DELAY);

        // Cast it back to normal data for T
        data = *(T*)output_data;
    }

    unsigned char Read(const uint8_t address)
    {
        // Set the address to read from
        uint8_t set_address[1] = { address };
        HAL_I2C_Master_Transmit(i2c, Write_Condition, set_address, 1,
            HAL_MAX_DELAY);

        // Read data
        unsigned char data;
        HAL_I2C_Master_Receive(i2c, Read_Condition, &data, 1, HAL_MAX_DELAY);
        return data;
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
            Write(&clear_bytes);
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
    inline void PerformWrite(unsigned char* data,
                             const unsigned short address,
                             const unsigned short data_size)
    {
        const size_t data_w_address = data_size + 1;

        // Copy to_write to a new array
        unsigned char bytes[data_w_address];
        bytes[0] = address;

        for (size_t i = 0; i < data_size; ++i)
        {
            bytes[i+1] = data[i];
        }

        // TODO rewrite hal transmit so I don't need to copy each message..
        // Write data
        HAL_I2C_Master_Transmit(i2c, Write_Condition, bytes, data_w_address,
            HAL_MAX_DELAY);
    }

    static constexpr uint8_t Read_Condition     = 0xA1;
    static constexpr uint8_t Write_Condition    = 0xA0;
    I2C_HandleTypeDef* i2c;
    const unsigned int max_sz;
    unsigned int next_address;
};