#pragma once

#include <logging.h>

namespace hactar_utils {


class ESP32SerialLogger : public LogHandler {

public:
  void log(const char* tag, const LogLevel level, const char* fmt, va_list args) override;
};

} // namespace