#include <hpke/signature.h>
#include <hpke/random.h>
#include <hpke/digest.h>
#include <namespace.h>
#include <string>

#include "common.h"

#include <crypto/ecc/cmox_ecc.h>
#include <crypto/ecc/cmox_ecdsa.h>

namespace MLS_NAMESPACE::hpke {

#define CURVE CMOX_ECC_SECP256R1_LOWMEM

// RAII wrapper for cmox_ecc_handle_t and its associated memory buffer.
struct ECCContext {
  // XXX Not at all clear what the right value for this parameter is.  This is
  // what it is set to in the examples.
  static constexpr size_t buffer_size = 2000;

  std::array<uint8_t, buffer_size> buffer;
  cmox_ecc_handle_t ctx;

  ECCContext() {
    cmox_ecc_construct(&ctx, CMOX_ECC256_MATH_FUNCS, buffer.data(), buffer.size());
  }

  cmox_ecc_handle_t* get() {
    return &ctx;
  }

  ~ECCContext() {
    cmox_ecc_cleanup(&ctx);
  }
};

struct P256Signature : Signature {
  struct PrivateKey : Signature::PrivateKey {
    bytes priv;
    bytes pub;

    PrivateKey(bytes priv_in, bytes pub_in)
      : priv(std::move(priv_in))
      , pub(std::move(pub_in))
    {}

    std::unique_ptr<Signature::PublicKey> public_key() const override {
      return std::make_unique<P256Signature::PublicKey>(pub);
    }
  };

  struct PublicKey : Signature::PublicKey {
    bytes pub;

    PublicKey(bytes pub_in)
      : pub(std::move(pub_in))
    {}
  };

  P256Signature()
    : Signature(ID::P256_SHA256)
  {}

  std::unique_ptr<Signature::PrivateKey> generate_key_pair() const override {
    const auto randomness = random_bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);
    return derive_key_pair(randomness);
  }

  std::unique_ptr<Signature::PrivateKey> derive_key_pair(const bytes& ikm) const override {
    // Key buffers
    auto priv = bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);
    auto priv_size = priv.size();

    auto pub = bytes(CMOX_ECC_SECP256R1_PUBKEY_LEN);
    auto pub_size = pub.size();

    /// vvv PROBLEM IS SOMEWHERE IN HERE vvv ///
    /*
    auto ctx = ECCContext();
    const auto rv = cmox_ecdsa_keyGen(ctx.get(),
                                      CMOX_ECC_SECP256R1_LOWMEM,
                                      ikm.data(),
                                      ikm.size(),
                                      priv.data(),
                                      &priv_size,
                                      pub.data(),
                                      &pub_size);

    if (rv != CMOX_ECC_SUCCESS) {
      throw CMOXError::from_code(rv);
    }
    */
    /// ^^^ PROBLEM IS SOMEWHERE IN HERE ^^^ ///

    priv.resize(priv_size);
    pub.resize(pub_size);

    return std::make_unique<P256Signature::PrivateKey>(std::move(priv), std::move(pub));
  }

  bytes serialize(const Signature::PublicKey& pk) const override {
    const auto& rpk = dynamic_cast<const P256Signature::PublicKey&>(pk);
    return rpk.pub;
  }

  std::unique_ptr<Signature::PublicKey> deserialize(const bytes& enc) const override {
    return std::make_unique<P256Signature::PublicKey>(enc);
  }

  bytes serialize_private(const Signature::PrivateKey& sk) const override {
    const auto& rsk = reinterpret_cast<const P256Signature::PrivateKey&>(sk);
    const auto len = std::vector<uint8_t>(1, uint8_t(rsk.priv.size()));
    return bytes(len) + rsk.priv + rsk.pub;
  }

  std::unique_ptr<Signature::PrivateKey> deserialize_private(const bytes& enc) const override {
    auto len = size_t(enc.data()[0]);
    auto priv = std::vector<uint8_t>(enc.begin() + 1, enc.begin() + len + 1);
    auto pub = std::vector<uint8_t>(enc.begin() + len + 1, enc.end());
    return std::make_unique<P256Signature::PrivateKey>(std::move(priv), std::move(pub));
  }

  bytes sign(const bytes& data, const Signature::PrivateKey& sk) const override {
    // XXX(RLB) This should use dynamic_cast, but dynamic_cast appears to throw
    // `bad_cast` on STM32.
    const auto& rsk = reinterpret_cast<const P256Signature::PrivateKey&>(sk);

    const auto sha256 = Digest::get<Digest::ID::SHA256>();
    const auto digest = sha256.hash(data);

    auto sig = bytes(CMOX_ECC_SECP256R1_SIG_LEN);
    auto sig_size = sig.size();

    auto ctx = ECCContext();
    const auto randomness = random_bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);
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
    const auto& rpk = reinterpret_cast<const P256Signature::PublicKey&>(pk);

    const auto sha256 = Digest::get<Digest::ID::SHA256>();
    const auto digest = sha256.hash(data);

    auto fault_check = uint32_t(0);
    auto ctx = ECCContext();
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

    return (fault_check == rv);
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
