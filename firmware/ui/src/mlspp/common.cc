#include "mls/common.h"
#include <namespace.h>

namespace MLS_NAMESPACE {

#if 0
uint64_t
seconds_since_epoch()
{
  // TODO(RLB) This should use std::chrono, but that seems not to be available
  // on some platforms.
  return std::time(nullptr);
}
#endif // 0

} // namespace MLS_NAMESPACE
