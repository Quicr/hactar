#include <hpke/base64.h>
#include <hpke/digest.h>
#include <hpke/signature.h>
#include <namespace.h>
#include <string>

#include "dhkem.h"

namespace MLS_NAMESPACE::hpke {

struct Ed25519 : Signature {
  struct PrivateKey : Signature::PrivateKey {
    bytes priv;
    bytes pub;

    std::unique_ptr<Signature::PublicKey> public_key() const override {
      return std::make_unique<Ed25519::PublicKey>();
    }
  };

  struct PublicKey : Signature::PublicKey {
  };

  Ed25519()
    : Signature(ID::Ed25519)
  {}

  std::unique_ptr<Signature::PrivateKey> generate_key_pair() const override {
     return std::make_unique<Ed25519::PrivateKey>(); // TODO
  }

  std::unique_ptr<Signature::PrivateKey> derive_key_pair(const bytes& ikm) const override {
     return std::make_unique<Ed25519::PrivateKey>(); // TODO
  }

  bytes serialize(const Signature::PublicKey& pk) const override {
     return {}; // TODO
  }

  std::unique_ptr<Signature::PublicKey> deserialize(const bytes& enc) const override {
     return std::make_unique<Ed25519::PublicKey>(); // TODO
  }

  bytes serialize_private(const Signature::PrivateKey& sk) const override {
     return {}; // TODO
  }

  std::unique_ptr<Signature::PrivateKey> deserialize_private(const bytes& skm) const override {
     return std::make_unique<Ed25519::PrivateKey>(); // TODO
  }

  bytes sign(const bytes& data, const Signature::PrivateKey& sk) const override {
     return {}; // TODO
  }

  bool verify(const bytes& data,
              const bytes& sig,
              const Signature::PublicKey& pk) const override {
     return false; // TODO
  }
};



} // namespace MLS_NAMESPACE::hpke
