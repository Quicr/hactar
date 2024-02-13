#include <hpke/signature.h>
#include <hpke/random.h>
#include <namespace.h>
#include <string>

#include "crypto/ecc/cmox_ecc.h"
#include "crypto/ecc/cmox_eddsa.h"

namespace MLS_NAMESPACE::hpke {

using PrivateKeyBuffer = std::array<uint8_t, CMOX_ECC_ED25519_PRIVKEY_LEN>;
using PublicKeyBuffer = std::array<uint8_t, CMOX_ECC_ED25519_PUBKEY_LEN>;

// RAII wrapper for cmox_ecc_handle_t and its associated memory buffer.
struct ECCContext {
  // XXX Not at all clear what the right value for this parameter is.
  static constexpr size_t buffer_size = 128;

  std::array<uint8_t, buffer_size> buffer;
  cmox_ecc_handle_t ctx;

  ECCContext() {
    cmox_ecc_construct(&ctx, CMOX_MATH_FUNCS_SMALL, buffer.data(), buffer.size());
  }

  cmox_ecc_handle_t* get() {
    return &ctx;
  }

  ~ECCContext() {
    cmox_ecc_cleanup(&ctx);
  }
};

struct Ed25519 : Signature {
  struct PrivateKey : Signature::PrivateKey {
    PrivateKeyBuffer priv;

    PrivateKey(PrivateKeyBuffer priv_in)
      : priv(std::move(priv_in))
    {}

    std::unique_ptr<Signature::PublicKey> public_key() const override {
      auto pub = PublicKeyBuffer();
      std::copy(priv.begin() + 32, priv.end(), pub.begin());
      return std::make_unique<Ed25519::PublicKey>(std::move(pub));
    }
  };

  struct PublicKey : Signature::PublicKey {
    PublicKeyBuffer pub;

    PublicKey(PublicKeyBuffer pub_in)
      : pub(std::move(pub_in))
    {}
  };

  Ed25519()
    : Signature(ID::Ed25519)
  {}

  std::unique_ptr<Signature::PrivateKey> generate_key_pair() const override {
    const auto randomness = random_bytes(CMOX_ECC_CURVE25519_SECRET_LEN);
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
    cmox_eddsa_keyGen(ctx.get(),
                      CMOX_ECC_ED25519_OPT_LOWMEM,
                      ikm.data(),
                      ikm.size(),
                      priv.data(),
                      &priv_size,
                      pub.data(),
                      &pub_size);


    return std::make_unique<Ed25519::PrivateKey>(std::move(priv));
  }

  bytes serialize(const Signature::PublicKey& pk) const override {
    const auto& rpk = dynamic_cast<const Ed25519::PublicKey&>(pk);
    return bytes(std::vector<uint8_t>(rpk.pub.begin(), rpk.pub.end()));
  }

  std::unique_ptr<Signature::PublicKey> deserialize(const bytes& enc) const override {
    auto pub = PublicKeyBuffer();
    std::copy(enc.begin(), enc.end(), pub.begin());
    return std::make_unique<Ed25519::PublicKey>(std::move(pub));
  }

  bytes serialize_private(const Signature::PrivateKey& sk) const override {
    const auto& rsk = dynamic_cast<const Ed25519::PrivateKey&>(sk);
    return bytes(std::vector<uint8_t>(rsk.priv.begin(), rsk.priv.end()));
  }

  std::unique_ptr<Signature::PrivateKey> deserialize_private(const bytes& enc) const override {
    auto priv = PrivateKeyBuffer();
    std::copy(enc.begin(), enc.end(), priv.begin());
    return std::make_unique<Ed25519::PrivateKey>(std::move(priv));
  }

  bytes sign(const bytes& data, const Signature::PrivateKey& sk) const override {
    const auto& rsk = dynamic_cast<const Ed25519::PrivateKey&>(sk);

    auto sig = bytes(CMOX_ECC_ED25519_SIG_LEN);
    auto sig_size = sig.size();

    // TODO check return value
    auto ctx = ECCContext{};
    cmox_eddsa_sign(ctx.get(),
                    CMOX_ECC_ED25519_OPT_LOWMEM,
                    rsk.priv.data(),
                    rsk.priv.size(),
                    data.data(),
                    data.size(),
                    sig.data(),
                    &sig_size);

    return sig;
  }

  bool verify(const bytes& data,
              const bytes& sig,
              const Signature::PublicKey& pk) const override {
    const auto& rpk = dynamic_cast<const Ed25519::PublicKey&>(pk);

    auto fault_check = uint32_t(0);
    auto ctx = ECCContext{};
    const auto rv = cmox_eddsa_verify(ctx.get(),
                                      CMOX_ECC_ED25519_OPT_LOWMEM,
                                      rpk.pub.data(),
                                      rpk.pub.size(),
                                      data.data(),
                                      data.size(),
                                      sig.data(),
                                      sig.size(),
                                      &fault_check);

    return (rv == CMOX_ECC_AUTH_SUCCESS) && (rv == fault_check);
  }
};

template<>
const Signature& Signature::get<Signature::ID::Ed25519>()
{
  static const Ed25519 instance;
  return instance;
}

Signature::Signature(ID id_in)
  : id(id_in)
{}


} // namespace MLS_NAMESPACE::hpke
