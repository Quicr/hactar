#include <hpke/random.h>
#include <namespace.h>

// Defined in `app_main.cc`
extern uint8_t rand_byte();

namespace MLS_NAMESPACE::hpke {

bytes
random_bytes(size_t size)
{
  auto rand = bytes(size);

  for (size_t i = 0; i < rand.size(); i++) {
    rand.data()[i] = rand_byte();
  }

  return rand;
}

} // namespace MLS_NAMESPACE::hpke
