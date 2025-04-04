/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_MAIN_HH
#define __APP_MAIN_HH

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
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

inline void LEDR(GPIO_PinState r);
inline void LEDG(GPIO_PinState g);
inline void LEDB(GPIO_PinState b);
inline void LEDS(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b);
inline void RaiseFlag(Timer_Flags flag);
inline void LowerFlag(Timer_Flags flag);
inline void LowPowerMode();
inline void WakeUp();
inline void CheckFlags();
inline void HandleRecvLinkPackets();
inline void ProcessText(uint16_t len);
inline void InitScreen();
inline void WaitForNetReady();
inline void AudioCallback();
inline void CheckPTT();
inline void CheckPTTAI();
inline void SendAudio(const uint8_t channel_id);
void Error(const char* who, const char* why);

void SlowSendTest(int delay, int num);
void InterHactarSerialRoundTripTest(int delay, int num);
void InterHactarFullRoundTripTest(int delay, int num);
void DumpRxBuff();

inline void LedROn();
inline void LedROff();
inline void LedRToggle();
inline void LedBOn();
inline void LedBOff();
inline void LedBToggle();
inline void LedGOn();
inline void LedGOff();
inline void LedGToggle();
inline void LedsOn();
inline void LedsOff();
inline void LedsToggle();

#ifdef __cplusplus
}
#endif

#endif /* __APP_MAIN_HH */
