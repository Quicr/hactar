#include "chip_control.h"

#include "io_control.h"
#include "main.h"

void NetBootloaderMode()
{
    ChangeToOutput(NET_BOOT_GPIO_Port, NET_BOOT_Pin);
    ChangeToOutput(NET_NRST_GPIO_Port, NET_NRST_Pin);

    // Bring the boot low for esp, bootloader mode (0)
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_RESET);

    // Power cycle
    PowerCycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);
}

void NetNormalMode()
{
    ChangeToOutput(NET_BOOT_GPIO_Port, NET_BOOT_Pin);
    ChangeToOutput(NET_NRST_GPIO_Port, NET_NRST_Pin);

    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);

    // Power cycle
    PowerCycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);

    // ChangeToInput(NET_BOOT_GPIO_Port, NET_BOOT_Pin);
    // ChangeToInput(NET_NRST_GPIO_Port, NET_NRST_Pin);
}

void NetHoldInReset()
{
    ChangeToOutput(NET_NRST_GPIO_Port, NET_NRST_Pin);
    // Reset
    HAL_GPIO_WritePin(NET_NRST_GPIO_Port, NET_NRST_Pin, GPIO_PIN_RESET);
}

void UIBootloaderMode()
{
    ChangeToOutput(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin);
    ChangeToOutput(UI_NRST_GPIO_Port, UI_NRST_Pin);

    // Normal boot mode (boot0 = 1 and boot1 = 0)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_RESET);

    // Power cycle
    PowerCycle(UI_NRST_GPIO_Port, UI_NRST_Pin, 10);
}

void UINormalMode()
{
    ChangeToOutput(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin);
    ChangeToOutput(UI_NRST_GPIO_Port, UI_NRST_Pin);

    // Normal boot mode (boot0 = 0 and boot1 = 1)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

    // Power cycle
    PowerCycle(UI_NRST_GPIO_Port, UI_NRST_Pin, 10);

    ChangeToInput(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin);
    ChangeToInput(UI_NRST_GPIO_Port, UI_NRST_Pin);
}

void UIHoldInReset()
{
    ChangeToOutput(UI_NRST_GPIO_Port, UI_NRST_Pin);

    HAL_GPIO_WritePin(UI_NRST_GPIO_Port, UI_NRST_Pin, GPIO_PIN_RESET);
}