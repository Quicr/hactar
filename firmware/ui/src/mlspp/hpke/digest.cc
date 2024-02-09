#include <hpke/digest.h>
#include <namespace.h>
#include <crypto/hash/cmox_hash.h>

namespace MLS_NAMESPACE::hpke {

cmox_hash_algo_t
cmox_hash_id(Digest::ID id) {
  switch (id) {
    case Digest::ID::SHA256: return CMOX_SHA256_ALGO;
    case Digest::ID::SHA384: return CMOX_SHA384_ALGO;
    case Digest::ID::SHA512: return CMOX_SHA512_ALGO;
  }
}

size_t
cmox_hash_size(Digest::ID id) {
  switch (id) {
    case Digest::ID::SHA256: return 32;
    case Digest::ID::SHA384: return 48;
    case Digest::ID::SHA512: return 64;
  }
}

Digest::Digest(Digest::ID id_in)
  : id(id_in)
  , hash_size(cmox_hash_size(id_in))
{
}

bytes
Digest::hash(const bytes& data) const
{
  auto md = bytes(hash_size);

  // TODO check return values and throw on error
  auto digest_len = size_t(0);
  cmox_hash_compute(cmox_hash_id(id),
                    data.data(),
                    data.size(),
                    md.data(),
                    md.size(),
                    &digest_len);

  return md;
}

bytes
Digest::hmac(const bytes& key, const bytes& data) const
{
  auto md = bytes(hash_size);
  unsigned int size = 0;
  return md;
}

bytes
Digest::hmac_for_hkdf_extract(const bytes& key, const bytes& data) const
{
  auto md = bytes(hash_size);
  return md;
}

} // namespace MLS_NAMESPACE::hpke
