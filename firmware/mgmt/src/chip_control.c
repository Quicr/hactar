#include "chip_control.h"
#include "app_mgmt.h"
#include "io_control.h"
#include "main.h"

void chip_control_net_bootloader_mode()
{
    io_control_power_cycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);
    // Bring the boot low for esp, bootloader mode (0)
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_RESET);

    // Power cycle
    io_control_power_cycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);
}

void chip_control_net_normal_mode()
{
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);

    // Power cycle
    io_control_power_cycle(NET_NRST_GPIO_Port, NET_NRST_Pin, 10);
}

void chip_control_net_hold_in_reset()
{
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);

    // Reset
    HAL_GPIO_WritePin(NET_NRST_GPIO_Port, NET_NRST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
}

void chip_control_normal_mode()
{
    chip_control_net_normal_mode();
    HAL_Delay(200);
    chip_control_ui_normal_mode();
}

void chip_control_ui_bootloader_mode()
{
    // Normal boot mode (boot0 = 1 and boot1 = 0)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_RESET);

    // Power cycle
    chip_control_ui_power_cycle();
}

void chip_control_ui_normal_mode()
{
    // Normal boot mode (boot0 = 0 and boot1 = 1)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

    // Power cycle
    chip_control_ui_power_cycle();
}

void chip_control_ui_hold_in_reset()
{
    // Normal boot mode (boot0 = 0 and boot1 = 1)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(UI_NRST_GPIO_Port, UI_NRST_Pin, GPIO_PIN_RESET);
}

void chip_control_ui_power_cycle()
{
    io_control_change_to_output(UI_NRST_GPIO_Port, UI_NRST_Pin);
    io_control_power_cycle(UI_NRST_GPIO_Port, UI_NRST_Pin, 10);
    HAL_GPIO_WritePin(UI_NRST_GPIO_Port, UI_NRST_Pin, GPIO_PIN_RESET);
    io_control_change_to_input(UI_NRST_GPIO_Port, UI_NRST_Pin);
}
