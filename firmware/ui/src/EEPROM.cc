#include "EEPROM.hh"

EEPROM::EEPROM(I2C_HandleTypeDef& hi2c) :
    i2c(&hi2c)
{
}

EEPROM::~EEPROM()
{
    i2c = nullptr;
}

void EEPROM::Write(uint8_t* data, const uint16_t sz)
{
    // Write data
    HAL_StatusTypeDef write_res = HAL_I2C_Master_Transmit(i2c, Write_Condition, data, sz, HAL_MAX_DELAY);
}

void EEPROM::Read(const uint8_t address, uint8_t* data, const uint16_t sz)
{
    // Set the address to read from
    uint8_t set_address[1] = { address };
    HAL_StatusTypeDef read_res = HAL_I2C_Master_Transmit(i2c, Write_Condition, set_address, 1, HAL_MAX_DELAY);

    // Read data
    HAL_I2C_Master_Receive(i2c, Read_Condition, data, sz, HAL_MAX_DELAY);
}