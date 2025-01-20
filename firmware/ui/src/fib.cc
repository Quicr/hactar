#include "main.h"
#include "fib.hh"

#include <string>

extern UART_HandleTypeDef huart1;

void SendResult(std::string str)
{

    HAL_UART_Transmit(&huart1, (const uint8_t*)str.c_str(), str.length(), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\n", 1, HAL_MAX_DELAY);
}

void FibMain()
{
    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);
    HAL_Delay(5000);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_RESET);

    constexpr int int_test = 34;
    constexpr int float_test = 32;
    uint32_t curr;
    uint32_t elapsed;
    std::string str;

    curr = HAL_GetTick();
    FibInt32(int_test);
    elapsed = HAL_GetTick() - curr;
    str = "[C] fib u32 " + std::to_string(int_test) + " -> " + std::to_string(elapsed) + "ms";
    SendResult(str);

    curr = HAL_GetTick();
    FibInt64(int_test);
    elapsed = HAL_GetTick() - curr;
    str = "[C] fib u64 " + std::to_string(int_test) + " -> " + std::to_string(elapsed) + "ms";
    SendResult(str);

    curr = HAL_GetTick();
    FibFloat32(float_test);
    elapsed = HAL_GetTick() - curr;
    str = "[C] fib f32 " + std::to_string(float_test) + " -> " + std::to_string(elapsed) + "ms";
    SendResult(str);

    curr = HAL_GetTick();
    FibFloat64(float_test);
    elapsed = HAL_GetTick() - curr;
    str = "[C] fib f64 " + std::to_string(float_test) + " -> " + std::to_string(elapsed) + "ms";
    SendResult(str);

    while (1)
    {

    }
}

int32_t FibInt32(const uint32_t n)
{
    if (n > 2)
    {
        return FibInt32(n-1) + FibInt32(n-2);
    }

    return (uint32_t)1;
}

int64_t FibInt64(const uint32_t n)
{
    if (n > 2)
    {
        return FibInt64(n-1) + FibInt64(n-2);
    }

    return (int64_t)1;
}

float FibFloat32(const uint32_t n)
{
    if (n > 2)
    {
        return FibFloat32(n-1) + FibFloat32(n-2);
    }

    return 1.0f;
}

double FibFloat64(const uint32_t n)
{
    if (n > 2)
    {
        return FibFloat64(n-1) + FibFloat64(n-2);
    }

    return 1.0;
}