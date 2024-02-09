#include "dhkem.h"

#include "common.h"
#include <namespace.h>

namespace MLS_NAMESPACE::hpke {

std::unique_ptr<KEM::PublicKey>
DHKEM::PrivateKey::public_key() const
{
  return nullptr;
}

DHKEM
make_dhkem(KEM::ID kem_id_in, const KDF& kdf_in)
{
  return { kem_id_in, kdf_in };
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_P256_SHA256>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_P256_SHA256,
                                          KDF::get<KDF::ID::HKDF_SHA256>());
  return instance;
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_P384_SHA384>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_P384_SHA384,
                                          KDF::get<KDF::ID::HKDF_SHA384>());
  return instance;
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_P521_SHA512>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_P521_SHA512,
                                          KDF::get<KDF::ID::HKDF_SHA512>());
  return instance;
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_X25519_SHA256>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_X25519_SHA256,
                                          KDF::get<KDF::ID::HKDF_SHA256>());
  return instance;
}

#if !defined(WITH_BORINGSSL)
template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_X448_SHA512>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_X448_SHA512,
                                          KDF::get<KDF::ID::HKDF_SHA512>());
  return instance;
}
#endif

DHKEM::DHKEM(KEM::ID kem_id_in, const KDF& kdf_in)
  : KEM(kem_id_in,
        kdf_in.hash_size,
        0, 0, 0)
  , kdf(kdf_in)
{
  static const auto label_kem = from_ascii("KEM");
  suite_id = label_kem + i2osp(uint16_t(kem_id_in), 2);
}

std::unique_ptr<KEM::PrivateKey>
DHKEM::generate_key_pair() const
{
  return nullptr;
}

std::unique_ptr<KEM::PrivateKey>
DHKEM::derive_key_pair(const bytes& ikm) const
{
  return nullptr;
}

bytes
DHKEM::serialize(const KEM::PublicKey& pk) const
{
  return {};
}

std::unique_ptr<KEM::PublicKey>
DHKEM::deserialize(const bytes& enc) const
{
  return nullptr;
}

bytes
DHKEM::serialize_private(const KEM::PrivateKey& sk) const
{
  const auto& gsk = dynamic_cast<const PrivateKey&>(sk);
  return {};
}

std::unique_ptr<KEM::PrivateKey>
DHKEM::deserialize_private(const bytes& skm) const
{
  return nullptr;
}

std::pair<bytes, bytes>
DHKEM::encap(const KEM::PublicKey& pkR) const
{
  return {};
}

bytes
DHKEM::decap(const bytes& enc, const KEM::PrivateKey& skR) const
{
  return {};
}

std::pair<bytes, bytes>
DHKEM::auth_encap(const KEM::PublicKey& pkR, const KEM::PrivateKey& skS) const
{
  return {};
}

bytes
DHKEM::auth_decap(const bytes& enc,
                  const KEM::PublicKey& pkS,
                  const KEM::PrivateKey& skR) const
{
  return {};
}

bytes
DHKEM::extract_and_expand(const bytes& dh, const bytes& kem_context) const
{
  static const auto label_eae_prk = from_ascii("eae_prk");
  static const auto label_shared_secret = from_ascii("shared_secret");

  auto eae_prk = kdf.labeled_extract(suite_id, {}, label_eae_prk, dh);
  return kdf.labeled_expand(
    suite_id, eae_prk, label_shared_secret, kem_context, secret_size);
}

} // namespace MLS_NAMESPACE::hpke
