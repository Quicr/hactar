#include <hpke/digest.h>
#include <namespace.h>
#include <crypto/hash/cmox_hash.h>
#include <crypto/mac/cmox_mac.h>
#include <crypto/mac/cmox_hmac.h>

namespace MLS_NAMESPACE::hpke {

cmox_hash_algo_t
cmox_hash_id(Digest::ID id) {
  switch (id) {
    case Digest::ID::SHA256: return CMOX_SHA256_ALGO;
    case Digest::ID::SHA384: return CMOX_SHA384_ALGO;
    case Digest::ID::SHA512: return CMOX_SHA512_ALGO;
  }
}

cmox_mac_algo_t
cmox_hmac_id(Digest::ID id) {
  switch (id) {
    case Digest::ID::SHA256: return CMOX_HMAC_SHA256_ALGO;
    case Digest::ID::SHA384: return CMOX_HMAC_SHA384_ALGO;
    case Digest::ID::SHA512: return CMOX_HMAC_SHA512_ALGO;
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

template<>
const Digest&
Digest::get<Digest::ID::SHA256>()
{
  static const Digest instance(Digest::ID::SHA256);
  return instance;
}

template<>
const Digest&
Digest::get<Digest::ID::SHA384>()
{
  static const Digest instance(Digest::ID::SHA384);
  return instance;
}

template<>
const Digest&
Digest::get<Digest::ID::SHA512>()
{
  static const Digest instance(Digest::ID::SHA512);
  return instance;
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
  auto md_size = md.size();

  // TODO check return value
  cmox_mac_compute(cmox_hmac_id(id),
                   data.data(),
                   data.size(),
                   key.data(),
                   key.size(),
                   nullptr,
                   0,
                   md.data(),
                   md.size(),
                   &md_size);

  md.resize(md_size);
  return md;
}

bytes
Digest::hmac_for_hkdf_extract(const bytes& key, const bytes& data) const
{
  return hmac(key, data);
}

} // namespace MLS_NAMESPACE::hpke
