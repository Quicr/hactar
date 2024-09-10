#include "common.h"
#include "p256.h"

namespace mls::hpke::p256 {

namespace {

#define CURVE CMOX_ECC_SECP256R1_LOWMEM

struct ECCContext {
  // XXX Not at all clear what the right value for this parameter is.
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
                    CURVE,
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
  return bytes(1, 0x04) + pk.pub;
}

PublicKey deserialize(const bytes& enc){
  return { enc.slice(1, enc.size()) };
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
                            CURVE,
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
                                  CURVE,
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

  const auto half_size = sig_size / 2;

  const auto r = sig.slice(0, half_size);
  auto r_header = bytes{0x02, static_cast<uint8_t>(r.size())};
  if (r.at(0) >= 0x80)
  {
    r_header.at(1) += 1;
    r_header.push_back(0x00);
  }

  const auto s = sig.slice(half_size, sig_size);
  auto s_header = bytes{0x02, static_cast<uint8_t>(s.size())};
  if (s.at(0) >= 0x80)
  {
    s_header.at(1) += 1;
    s_header.push_back(0x00);
  }

  const auto sequence_data = r_header + r + s_header + s;
  const auto sequence_header = bytes{0x30, static_cast<uint8_t>(sequence_data.size())};

  return sequence_header + sequence_data;
}

bool verify(const bytes& data, const bytes& sig, const PublicKey& pk) {
  const auto sha256 = Digest::get<Digest::ID::SHA256>();
  const auto digest = sha256.hash(data);

  static constexpr auto sequence_header_size = 2;
  static constexpr auto sig_int_size = 32;

  const auto sequence_data = sig.slice(sequence_header_size, sig.size());

  const auto r_size = sequence_data.at(1);
  const auto r_data_start = sequence_header_size;
  auto r = sequence_data.slice(r_data_start, r_data_start + r_size);

  if (r_size > sig_int_size) { r = r.slice(1, r.size()); }

  const auto s_size = sequence_data.at(sequence_header_size + r_size + 1);
  const auto s_data_start = sequence_header_size * 2 +  r_size;
  auto s = sequence_data.slice(s_data_start, sequence_data.size());

  if (s_size > sig_int_size) { s = s.slice(1, s.size()); }

  const auto real_sig = r + s;

  auto fault_check = uint32_t(0);
  auto ctx = ECCContext();
  const auto rv = cmox_ecdsa_verify(ctx.get(),
                                    CMOX_ECC_SECP256R1_LOWMEM,
                                    pk.pub.data(),
                                    pk.pub.size(),
                                    digest.data(),
                                    digest.size(),
                                    real_sig.data(),
                                    real_sig.size(),
                                    &fault_check);

  if (rv != CMOX_ECC_AUTH_SUCCESS) {
    throw CMOXError::from_code(rv);
  }

  return (fault_check == rv);
}

} // namespace mls::hpke::p256
