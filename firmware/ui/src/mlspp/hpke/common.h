#pragma once

#include <hpke/hpke.h>
#include <namespace.h>
#include <stdexcept>

namespace MLS_NAMESPACE::hpke {

bytes
i2osp(uint64_t val, size_t size);


class CMOXError : std::runtime_error {
  using parent = std::runtime_error;
  using parent::parent;

  public:
  static CMOXError from_code(uint32_t rv);
};

} // namespace MLS_NAMESPACE::hpke
