#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "serial_logger.h"

using namespace hactar_utils;

static SemaphoreHandle_t xSemaphore = nullptr;

// see esp_log.h for the details
const auto log_color_red    =  31;
const auto log_color_green  =  32;
const auto log_color_brown  =  33;
const auto log_color_blue   =  34;
const auto log_color_purple =  35;
const auto log_color_cyan   =  36;

constexpr char level_code(const LogLevel level) {
  switch (level) {
    case LogLevel::Error:
        return 'E';
    case LogLevel::Warn:
        return 'W';
    case LogLevel::Info:
        return 'I';
    case LogLevel::Debug:
        return 'D';
    case LogLevel::Verbose:
        return 'V';
    default:
        return 'U';
  }
}

constexpr int color_code(const LogLevel level) {
  switch (level)
  {
    case LogLevel::Error:
        return log_color_red;
    case LogLevel::Warn:
        return log_color_purple;
    case LogLevel::Info:
        return log_color_green;
    case LogLevel::Debug:
        return log_color_cyan;
    case LogLevel::Verbose:
        return log_color_blue;
    default:
        return log_color_green;
  }
} 

esp_log_level_t to_esp_log_level(const LogLevel level) {
  switch (level)
  {
    case LogLevel::None:
        return ESP_LOG_NONE;
    case LogLevel::Error:
        return ESP_LOG_ERROR;
    case LogLevel::Warn:
        return ESP_LOG_WARN;
    case LogLevel::Info:
        return ESP_LOG_INFO;
    case LogLevel::Debug:
        return ESP_LOG_DEBUG;
    case LogLevel::Verbose:
        return ESP_LOG_VERBOSE;
    default:
        return ESP_LOG_INFO;
  }  
}

// TODO: run this in its own task/thread
void ESP32SerialLogger::log(const char* tag, const LogLevel level, const char* fmt, va_list args) {
    if (xSemaphore == nullptr) {
        xSemaphore = xSemaphoreCreateMutex();
    }

    if (xSemaphoreCreateMutex() != nullptr && xSemaphoreTake(xSemaphore, (TickType_t) 20)) {
        auto esp_level = to_esp_log_level(level);
        esp_log_write(esp_level, tag, "\033[0;%dm%c (%ld) %s:", color_code(level), level_code(level), esp_log_timestamp(), tag);
        esp_log_writev(esp_level, tag, fmt, args);
        esp_log_write(esp_level, tag, "\033[0m");
        xSemaphoreGive(xSemaphore);
    }
}