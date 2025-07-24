#ifndef CHIP_CONTROL_H
#define CHIP_CONTROL_H

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