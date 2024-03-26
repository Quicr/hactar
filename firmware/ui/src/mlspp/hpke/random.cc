#include <hpke/random.h>
#include <namespace.h>

// Defined in `app_main.cc` so that it can access the RNG peripheral
extern uint8_t random_byte();

namespace MLS_NAMESPACE::hpke {

bytes
random_bytes(size_t size)
{
  auto rand = bytes(size);

  for (size_t i = 0; i < rand.size(); i++) {
    rand.data()[i] = random_byte();
  }

  return rand;
}

} // namespace MLS_NAMESPACE::hpke
