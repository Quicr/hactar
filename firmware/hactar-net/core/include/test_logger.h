#pragma once
#include <mutex>
#include <transport/logger.h>

namespace hactar_net {

class TestLogger : public qtransport::LogHandler {
public:
  void log(qtransport::LogLevel level, const std::string &string) override;

private:
  std::mutex mutex;
};

}