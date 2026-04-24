#ifndef CHIP_CONTROL_H
#define CHIP_CONTROL_H

void chip_control_net_bootloader_mode();
void chip_control_net_normal_mode();
void chip_control_net_hold_in_reset();

void chip_control_normal_mode();

void chip_control_ui_bootloader_mode();
void chip_control_ui_normal_mode();
void chip_control_ui_hold_in_reset();
void chip_control_ui_power_cycle();

void UIBootloaderMode();
void UINormalMode();
void UIHoldInReset();
void UIPowerCycle();

void NetBootloaderMode();
void NetNormalMode();
void NetHoldInReset();

void NormalStart();

void WaitForNetReady();

#endif