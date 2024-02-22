#include <hpke/signature.h>
#include <hpke/random.h>
#include <hpke/digest.h>
#include <namespace.h>
#include <string>

#include "common.h"

#include <crypto/ecc/cmox_ecc.h>
#include <crypto/ecc/cmox_ecdsa.h>

namespace MLS_NAMESPACE::hpke {

#define MATH CMOX_MATH_FUNCS_SMALL
#define CURVE CMOX_ECC_SECP256R1_LOWMEM

using PrivateKeyBuffer = std::array<uint8_t, CMOX_ECC_SECP256R1_PRIVKEY_LEN>;
using PublicKeyBuffer = std::array<uint8_t, CMOX_ECC_SECP256R1_PUBKEY_LEN>;

// RAII wrapper for cmox_ecc_handle_t and its associated memory buffer.
struct ECCContext {
  // XXX Not at all clear what the right value for this parameter is.  This is
  // what it is set to in the examples.
  static constexpr size_t buffer_size = 2000;

  std::array<uint8_t, buffer_size> buffer;
  cmox_ecc_handle_t ctx;

  ECCContext() {
    cmox_ecc_construct(&ctx, MATH, buffer.data(), buffer.size());
  }

  cmox_ecc_handle_t* get() {
    return &ctx;
  }

  ~ECCContext() {
    cmox_ecc_cleanup(&ctx);
  }
};

struct P256 : Signature {
  struct PrivateKey : Signature::PrivateKey {
    PrivateKeyBuffer priv;

    PrivateKey(PrivateKeyBuffer priv_in)
      : priv(std::move(priv_in))
    {}

    std::unique_ptr<Signature::PublicKey> public_key() const override {
      auto pub = PublicKeyBuffer();
      std::copy(priv.begin() + 32, priv.end(), pub.begin());
      return std::make_unique<P256::PublicKey>(std::move(pub));
    }
  };

  struct PublicKey : Signature::PublicKey {
    PublicKeyBuffer pub;

    PublicKey(PublicKeyBuffer pub_in)
      : pub(std::move(pub_in))
    {}
  };

  P256()
    : Signature(ID::P256_SHA256)
  {}

  std::unique_ptr<Signature::PrivateKey> generate_key_pair() const override {
    const auto randomness = random_bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);
    return derive_key_pair(randomness);
  }

  std::unique_ptr<Signature::PrivateKey> derive_key_pair(const bytes& ikm) const override {
    // Private key buffer seed || pubkey
    auto priv = PrivateKeyBuffer{};
    auto priv_size = priv.size();

    // Public key buffer.  Ultimately discarded, because it is duplicative: The
    // public key is stored in the private key buffer.
    auto pub = PublicKeyBuffer{};
    auto pub_size = pub.size();

    // XXX It would be nice to store this context on the object, to facilitate
    // reuse.  But the various methods here are marked `const`, so we would have
    // to do some chicanery to hide the mutability of the context.
    auto ctx = ECCContext{};

    // TODO check return value
    const auto rv = cmox_ecdsa_keyGen(ctx.get(),
                                      CURVE,
                                      ikm.data(),
                                      ikm.size(),
                                      priv.data(),
                                      &priv_size,
                                      pub.data(),
                                      &pub_size);

    if (rv != CMOX_ECC_SUCCESS) {
      throw CMOXError::from_code(rv);
    }

    return std::make_unique<P256::PrivateKey>(std::move(priv));
  }

  bytes serialize(const Signature::PublicKey& pk) const override {
    const auto& rpk = dynamic_cast<const P256::PublicKey&>(pk);
    return bytes(std::vector<uint8_t>(rpk.pub.begin(), rpk.pub.end()));
  }

  std::unique_ptr<Signature::PublicKey> deserialize(const bytes& enc) const override {
    auto pub = PublicKeyBuffer();
    std::copy(enc.begin(), enc.end(), pub.begin());
    return std::make_unique<P256::PublicKey>(std::move(pub));
  }

  bytes serialize_private(const Signature::PrivateKey& sk) const override {
    const auto& rsk = reinterpret_cast<const P256::PrivateKey&>(sk);
    return bytes(std::vector<uint8_t>(rsk.priv.begin(), rsk.priv.end()));
  }

  std::unique_ptr<Signature::PrivateKey> deserialize_private(const bytes& enc) const override {
    auto priv = PrivateKeyBuffer();
    std::copy(enc.begin(), enc.end(), priv.begin());
    return std::make_unique<P256::PrivateKey>(std::move(priv));
  }

  bytes sign(const bytes& data, const Signature::PrivateKey& sk) const override {
    // XXX(RLB) This should use dynamic_cast, but dynamic_cast appears to throw
    // `bad_cast` on STM32.
    const auto& rsk = reinterpret_cast<const P256::PrivateKey&>(sk);

    const auto randomness = random_bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);

    const auto sha256 = Digest::get<Digest::ID::SHA256>();
    const auto digest = sha256.hash(data);

    auto sig = bytes(CMOX_ECC_SECP256R1_SIG_LEN);
    auto sig_size = sig.size();

    auto ctx = ECCContext{};
    const auto rv = cmox_ecdsa_sign(ctx.get(),
                                    CURVE,
                                    randomness.data(),
                                    randomness.size(),
                                    rsk.priv.data(),
                                    rsk.priv.size(),
                                    digest.data(),
                                    digest.size(),
                                    sig.data(),
                                    &sig_size);

    if (rv != CMOX_ECC_SUCCESS) {
      throw CMOXError::from_code(rv);
    }

    sig.resize(sig_size);
    return sig;
  }

  bool verify(const bytes& data,
              const bytes& sig,
              const Signature::PublicKey& pk) const override {
    const auto& rpk = reinterpret_cast<const P256::PublicKey&>(pk);

    const auto sha256 = Digest::get<Digest::ID::SHA256>();
    const auto digest = sha256.hash(data);

    auto fault_check = uint32_t(0);
    auto ctx = ECCContext{};
    const auto rv = cmox_ecdsa_verify(ctx.get(),
                                      CURVE,
                                      rpk.pub.data(),
                                      rpk.pub.size(),
                                      digest.data(),
                                      digest.size(),
                                      sig.data(),
                                      sig.size(),
                                      &fault_check);

    if (rv != CMOX_ECC_AUTH_SUCCESS) {
      throw CMOXError::from_code(rv);
    }

    return (rv == CMOX_ECC_AUTH_SUCCESS) && (rv == fault_check);
  }
};

template<>
const Signature& Signature::get<Signature::ID::P256_SHA256>()
{
  static const P256 instance;
  return instance;
}

Signature::Signature(ID id_in)
  : id(id_in)
{}


} // namespace MLS_NAMESPACE::hpke
