#include "common.h"
#include "p256.h"

namespace mls::hpke::p256 {

namespace {

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

} // namespace

PrivateKey derive_key_pair(const bytes& ikm){
  // Private key buffer seed || pubkey
  auto priv = bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);
  auto priv_size = priv.size();

  // Public key buffer.  Ultimately discarded, because it is duplicative: The
  // public key is stored in the private key buffer.
  auto pub = bytes(CMOX_ECC_SECP256R1_PUBKEY_LEN);
  auto pub_size = pub.size();

  // XXX It would be nice to store this context on the object, to facilitate
  // reuse.  But the various methods here are marked `const`, so we would have
  // to do some chicanery to hide the mutability of the context.
  auto ctx = ECCContext{};

  // TODO check return value
  cmox_ecdsa_keyGen(ctx.get(),
                    CMOX_ECC_SECP256R1_LOWMEM,
                    ikm.data(),
                    ikm.size(),
                    priv.data(),
                    &priv_size,
                    pub.data(),
                    &pub_size);


  return { std::move(priv), std::move(pub) };
}

PrivateKey generate_key_pair(){
  const auto randomness = random_bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);
  return derive_key_pair(randomness);
}

bytes serialize(const PublicKey& pk){
  return pk.pub;
}

PublicKey deserialize(const bytes& enc){
  return { enc };
}

bytes serialize_private(const PrivateKey& sk){
  const auto len = std::vector<uint8_t>(1, uint8_t(sk.priv.size()));
  return bytes(len) + sk.priv + sk.pub;
}

PrivateKey deserialize_private(const bytes& enc){
    auto len = size_t(enc.data()[0]);
    auto priv = std::vector<uint8_t>(enc.begin() + 1, enc.begin() + len + 1);
    auto pub = std::vector<uint8_t>(enc.begin() + len + 1, enc.end());
    return PrivateKey{ std::move(priv), std::move(pub) };
}

bytes dh(const p256::PrivateKey& sk, const p256::PublicKey& pk) {
  auto zz = bytes(CMOX_ECC_SECP256R1_SECRET_LEN);
  auto zz_size = zz.size();

  auto ctx = ECCContext{};
  const auto rv = cmox_ecdh(ctx.get(),
                            CMOX_ECC_SECP256R1_LOWMEM,
                            sk.priv.data(),
                            sk.priv.size(),
                            pk.pub.data(),
                            pk.pub.size(),
                            zz.data(),
                            &zz_size);

  if (rv != CMOX_ECC_SUCCESS) {
    throw CMOXError::from_code(rv);
  }

  zz.resize(zz_size);
  return zz;
}

bytes sign(const bytes& data, const PrivateKey& sk) {
  const auto sha256 = Digest::get<Digest::ID::SHA256>();
  const auto digest = sha256.hash(data);

  auto sig = bytes(CMOX_ECC_SECP256R1_SIG_LEN);
  auto sig_size = sig.size();

  auto ctx = ECCContext();
  const auto randomness = random_bytes(CMOX_ECC_SECP256R1_PRIVKEY_LEN);
  const auto rv = cmox_ecdsa_sign(ctx.get(),
                                  CMOX_ECC_SECP256R1_LOWMEM,
                                  randomness.data(),
                                  randomness.size(),
                                  sk.priv.data(),
                                  sk.priv.size(),
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

bool verify(const bytes& data, const bytes& sig, const PublicKey& pk) {
  const auto sha256 = Digest::get<Digest::ID::SHA256>();
  const auto digest = sha256.hash(data);

  auto fault_check = uint32_t(0);
  auto ctx = ECCContext();
  const auto rv = cmox_ecdsa_verify(ctx.get(),
                                    CMOX_ECC_SECP256R1_LOWMEM,
                                    pk.pub.data(),
                                    pk.pub.size(),
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

} // namespace mls::hpke::p256
