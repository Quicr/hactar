#pragma once
#include <optional>

namespace qtransport {

/**
 * @brief Transport logger level
 */
enum class LogLevel : uint8_t
{
  fatal = 1,
  error,
  warn,
  info,
  debug,
};

/**
 * @brief Transport log handler
 *
 * @details Transport log handler defines callback methods that can be
 * implemented by the application.
 */
class LogHandler
{
public:
  /**
   * @brief Transport log callback
   *
   * @details Applications can implement this method to receive log messages
   * from the transport. This is optional.
   *
   * @param level			Severity level as defined by
   * qtransport::LogLevel enum
   * @param message 	Log messages to log
   */
  virtual void log(LogLevel /*level*/, const std::string& /*message*/) {}
};
} // namespace qtransport