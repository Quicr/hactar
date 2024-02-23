#include <hpke/signature.h>
#include <hpke/random.h>
#include <hpke/digest.h>
#include <namespace.h>
#include <string>

#include "common.h"
#include "p256.h"

namespace MLS_NAMESPACE::hpke {

struct P256Signature : Signature {
  struct PrivateKey : Signature::PrivateKey {
    p256::PrivateKey priv;

    PrivateKey(p256::PrivateKey priv_in)
      : priv(priv_in)
    {}

    std::unique_ptr<Signature::PublicKey> public_key() const override {
      return std::make_unique<P256Signature::PublicKey>(p256::PublicKey{ priv.pub });
    }
  };

  struct PublicKey : Signature::PublicKey {
    p256::PublicKey pub;

    PublicKey(p256::PublicKey pub_in)
      : pub(pub_in)
    {}
  };

  P256Signature()
    : Signature(ID::P256_SHA256)
  {}

  std::unique_ptr<Signature::PrivateKey> generate_key_pair() const override {
    return std::make_unique<PrivateKey>(p256::generate_key_pair());
  }

  std::unique_ptr<Signature::PrivateKey> derive_key_pair(const bytes& ikm) const override {
    return std::make_unique<PrivateKey>(p256::derive_key_pair(ikm));
  }

  bytes serialize(const Signature::PublicKey& pk) const override {
    const auto& rpk = dynamic_cast<const P256Signature::PublicKey&>(pk);
    return p256::serialize(rpk.pub);
  }

  std::unique_ptr<Signature::PublicKey> deserialize(const bytes& enc) const override {
    return std::make_unique<PublicKey>(p256::deserialize(enc));
  }

  bytes serialize_private(const Signature::PrivateKey& sk) const override {
    const auto& rsk = dynamic_cast<const P256Signature::PrivateKey&>(sk);
    return p256::serialize_private(rsk.priv);
  }

  std::unique_ptr<Signature::PrivateKey> deserialize_private(const bytes& enc) const override {
    return std::make_unique<PrivateKey>(p256::deserialize_private(enc));
  }

  bytes sign(const bytes& data, const Signature::PrivateKey& sk) const override {
    const auto& rsk = dynamic_cast<const P256Signature::PrivateKey&>(sk);
    return p256::sign(data, rsk.priv);
  }

  bool verify(const bytes& data,
              const bytes& sig,
              const Signature::PublicKey& pk) const override {
    const auto& rpk = dynamic_cast<const P256Signature::PublicKey&>(pk);
    return p256::verify(data, sig, rpk.pub);
  }
};

template<>
const Signature& Signature::get<Signature::ID::P256_SHA256>()
{
  static const P256Signature instance;
  return instance;
}

Signature::Signature(ID id_in)
  : id(id_in)
{}


} // namespace MLS_NAMESPACE::hpke
