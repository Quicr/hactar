#include <mls/common.h>
#include <namespace.h>

namespace MLS_NAMESPACE {

uint64_t
seconds_since_epoch()
{
  /// XXX(BER) given that the time returned has issues on an stm32 using
  // std::time(nullptr); we are just hardcoding a value for now.
  // but in the future it needs to be tired to a RTC
  return uint64_t(0x00'00'00'00'ff'ff'ff'ff);

  // TODO(RLB) This should use std::chrono, but that seems not to be available
  // on some platforms.
  return std::time(nullptr);
}

} // namespace MLS_NAMESPACE
