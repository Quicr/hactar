#include <hpke/digest.h>
#include <namespace.h>

namespace MLS_NAMESPACE::hpke {

Digest::Digest(Digest::ID id_in)
  : id(id_in)
  , hash_size(0)
{
}

bytes
Digest::hash(const bytes& data) const
{
  auto md = bytes(hash_size);
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
