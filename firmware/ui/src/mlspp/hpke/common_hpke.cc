#include "common.h"
#include <namespace.h>

#include <crypto/cipher/cmox_cipher_retvals.h>
#include <crypto/ecc/cmox_ecc_retvals.h>

namespace MLS_NAMESPACE::hpke {

bytes
i2osp(uint64_t val, size_t size)
{
  auto out = bytes(size, 0);
  auto max = size;
  if (size > 8) {
    max = 8;
  }

  for (size_t i = 0; i < max; i++) {
    out.at(size - i - 1) = static_cast<uint8_t>(val >> (8 * i));
  }
  return out;
}

CMOXError
CMOXError::from_code(uint32_t rv)
{
  switch (rv) {
    // Cipher codes
    case CMOX_CIPHER_SUCCESS: return CMOXError("CMOX_CIPHER_SUCCESS");
    case CMOX_CIPHER_ERR_INTERNAL: return CMOXError("CMOX_CIPHER_ERR_INTERNAL");
    case CMOX_CIPHER_ERR_NOT_IMPLEMENTED: return CMOXError("CMOX_CIPHER_ERR_NOT_IMPLEMENTED");
    case CMOX_CIPHER_ERR_BAD_PARAMETER: return CMOXError("CMOX_CIPHER_ERR_BAD_PARAMETER");
    case CMOX_CIPHER_ERR_BAD_OPERATION: return CMOXError("CMOX_CIPHER_ERR_BAD_OPERATION");
    case CMOX_CIPHER_ERR_BAD_INPUT_SIZE: return CMOXError("CMOX_CIPHER_ERR_BAD_INPUT_SIZE");
    case CMOX_CIPHER_AUTH_SUCCESS: return CMOXError("CMOX_CIPHER_AUTH_SUCCESS");
    case CMOX_CIPHER_AUTH_FAIL: return CMOXError("CMOX_CIPHER_AUTH_FAIL");

    // ECC codes
    case CMOX_ECC_SUCCESS: return CMOXError("CMOX_ECC_SUCCESS");
    case CMOX_ECC_ERR_INTERNAL: return CMOXError("CMOX_ECC_ERR_INTERNAL");
    case CMOX_ECC_ERR_BAD_PARAMETERS: return CMOXError("CMOX_ECC_ERR_BAD_PARAMETERS");
    case CMOX_ECC_ERR_INVALID_PUBKEY: return CMOXError("CMOX_ECC_ERR_INVALID_PUBKEY");
    case CMOX_ECC_ERR_INVALID_SIGNATURE: return CMOXError("CMOX_ECC_ERR_INVALID_SIGNATURE");
    case CMOX_ECC_ERR_WRONG_RANDOM: return CMOXError("CMOX_ECC_ERR_WRONG_RANDOM");
    case CMOX_ECC_ERR_MEMORY_FAIL: return CMOXError("CMOX_ECC_ERR_MEMORY_FAIL");
    case CMOX_ECC_ERR_MATHCURVE_MISMATCH: return CMOXError("CMOX_ECC_ERR_MATHCURVE_MISMATCH");
    case CMOX_ECC_ERR_ALGOCURVE_MISMATCH: return CMOXError("CMOX_ECC_ERR_ALGOCURVE_MISMATCH");
    case CMOX_ECC_AUTH_SUCCESS: return CMOXError("CMOX_ECC_AUTH_SUCCESS");
    case CMOX_ECC_AUTH_FAIL: return CMOXError("CMOX_ECC_AUTH_FAIL");

    // Catch-all
    default: return CMOXError(std::string("Unknown: ") + std::to_string(rv));
  }
}

} // namespace MLS_NAMESPACE::hpke
