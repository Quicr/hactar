#include "chip_control.h"
#include "app_mgmt.h"
#include "io_control.h"
#include "main.h"

void NetBootloaderMode()
{
    PowerCycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);
    // Bring the boot low for esp, bootloader mode (0)
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_RESET);

    // Power cycle
    PowerCycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);
}

void NetNormalMode()
{
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);

    // Power cycle
    PowerCycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);
    // HAL_Delay(10);
}

void NetHoldInReset()
{
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);

    // Reset
    HAL_GPIO_WritePin(NET_NRST_GPIO_Port, NET_NRST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
}

void UIBootloaderMode()
{
    // Normal boot mode (boot0 = 1 and boot1 = 0)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_RESET);

    // Power cycle
    UIPowerCycle();
}

void UINormalMode()
{
    // Normal boot mode (boot0 = 0 and boot1 = 1)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

    // Power cycle
    UIPowerCycle();
}

void UIHoldInReset()
{
    // Normal boot mode (boot0 = 0 and boot1 = 1)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(UI_NRST_GPIO_Port, UI_NRST_Pin, GPIO_PIN_RESET);
}

void UIPowerCycle()
{
    ChangeToOutput(UI_NRST_GPIO_Port, UI_NRST_Pin);
    PowerCycle(UI_NRST_GPIO_Port, UI_NRST_Pin, 10);
    HAL_GPIO_WritePin(UI_NRST_GPIO_Port, UI_NRST_Pin, GPIO_PIN_RESET);
    ChangeToInput(UI_NRST_GPIO_Port, UI_NRST_Pin);
}

void NormalStart()
{
    // HAL_GPIO_WritePin(UI_STAT_GPIO_Port, UI_STAT_Pin, LOW);
    NetNormalMode();
    HAL_Delay(200);
    UINormalMode();
}

/**
 * @brief Waits for the network chip to send a ready signal
 *
 */
void WaitForNetReady(const enum State* state)
{
    // // Read from the Net chip
    // uint32_t timeout = HAL_GetTick() + 5000;
    // while (*state != Reset
    //     && HAL_GetTick() < timeout
    //     && HAL_GPIO_ReadPin(NET_STAT_GPIO_Port, NET_STAT_Pin) != GPIO_PIN_SET
    //     )
    // {
    //     // Stay here until the Net is done booting
    //     HAL_Delay(10);
    // }

    // HAL_GPIO_WritePin(UI_STAT_GPIO_Port, UI_STAT_Pin, HIGH);
}
