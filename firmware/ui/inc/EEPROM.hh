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
    const unsigned int Write(T* data, const uint16_t sz=1)
    {
        const unsigned short addr = next_address;
        const size_t data_size = sizeof(data) * sz;
        const size_t data_w_address = data_size + 1;
        unsigned char* to_write = (unsigned char*)(void*)&data;

        // Copy to_write to a new array
        unsigned char bytes[data_w_address];
        bytes[0] = addr;

        for (size_t i = 0; i < data_size; ++i)
        {
            bytes[i+1] = to_write[i];
        }

        // TODO rewrite hal transmit so I don't need to copy each message..
        // Write data
        HAL_StatusTypeDef write_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, bytes, data_w_address, HAL_MAX_DELAY);

        next_address += data_size;
        return addr;
    }

    template<typename T>
    void Read(const uint8_t address, T& data, const uint16_t sz=1)
    {
        // Set the address to read from
        uint8_t set_address[1] = { address };
        HAL_StatusTypeDef read_res = HAL_I2C_Master_Transmit(i2c,
            Write_Condition, set_address, 1, HAL_MAX_DELAY);

        // Read data
        unsigned short output_sz = sizeof(data) * sz;
        unsigned char* output_data = (unsigned char*)(void*)&data;
        HAL_I2C_Master_Receive(i2c, Read_Condition, output_data, output_sz,
            HAL_MAX_DELAY);

        // Cast it back to normal data for T
        data = *(T*)output_data;
    }

    const unsigned int Size() const
    {
        return max_sz;
    }

    const unsigned int NextAddress() const
    {
        return next_address;
    }

private:
    static constexpr uint8_t Read_Condition     = 0xA1;
    static constexpr uint8_t Write_Condition    = 0xA0;
    I2C_HandleTypeDef* i2c;
    const unsigned int max_sz;
    unsigned int next_address;
};