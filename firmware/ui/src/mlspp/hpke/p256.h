#pragma once

#include "random.h"

#include <hpke/digest.h>

#include <crypto/ecc/cmox_ecc.h>
#include <crypto/ecc/cmox_ecdh.h>
#include <crypto/ecc/cmox_ecdsa.h>

namespace mls::hpke::p256 {

struct PublicKey {
  bytes pub;
};

struct PrivateKey {
  bytes priv;
  bytes pub;

  PublicKey public_key() const {
    return { pub };
  }
};

PrivateKey derive_key_pair(const bytes& ikm);
PrivateKey generate_key_pair();

bytes serialize(const PublicKey& pk);
PublicKey deserialize(const bytes& enc);

bytes serialize_private(const PrivateKey& sk);
PrivateKey deserialize_private(const bytes& enc);

bytes dh(const p256::PrivateKey& sk, const p256::PublicKey& pk);

bytes sign(const bytes& data, const PrivateKey& sk);
bool verify(const bytes& data, const bytes& sig, const PublicKey& pk);


} // namespace mls::hpke::p256
