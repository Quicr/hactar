#ifndef __APP_MAIN_HH
#define __APP_MAIN_HH

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32.h"

enum Timer_Flags
{
    Audio_Interrupt = 0,
    Rx_Audio_Companded,
    Rx_Audio_Transmitted,
    Draw_Complete,
    Timer_Flags_Count
};

int app_main();

void RaiseFlag(enum Timer_Flags flag);
void LowerFlag(enum Timer_Flags flag);
void LowPowerMode();
void WakeUp();
void CheckFlags();
void WaitForNetReady();
void AudioCallback();
void Error(const char* who, const char* why);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MAIN_HH */
