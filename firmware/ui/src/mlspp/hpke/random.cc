#include <hpke/random.h>
#include <namespace.h>

namespace MLS_NAMESPACE::hpke {

bytes
random_bytes(size_t size)
{
  auto rand = bytes(size);
#if 0 // TODO
  if (1 != RAND_bytes(rand.data(), static_cast<int>(size))) {
    throw openssl_error();
  }
#endif // 0
  return rand;
}

} // namespace MLS_NAMESPACE::hpke
