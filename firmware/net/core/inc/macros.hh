#pragma once

#include "esp_timer.h"

#ifndef esp_timer_get_time_ms
#define esp_timer_get_time_ms() (esp_timer_get_time() / 1000)
#endif