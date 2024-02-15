#include "aead_cipher.h"
#include "common.h"

#include <namespace.h>

#include <crypto/cipher/cmox_cipher.h>
#include <crypto/cipher/cmox_gcm.h>

namespace MLS_NAMESPACE::hpke {

///
/// ExportOnlyCipher
///
bytes
ExportOnlyCipher::seal(const bytes& /* key */,
                       const bytes& /* nonce */,
                       const bytes& /* aad */,
                       const bytes& /* pt */) const
{
  throw std::runtime_error("seal() on export-only context");
}

std::optional<bytes>
ExportOnlyCipher::open(const bytes& /* key */,
                       const bytes& /* nonce */,
                       const bytes& /* aad */,
                       const bytes& /* ct */) const
{
  throw std::runtime_error("open() on export-only context");
}

ExportOnlyCipher::ExportOnlyCipher()
  : AEAD(AEAD::ID::export_only, 0, 0)
{
}

///
/// AEADCipher
///
AEADCipher
make_aead(AEAD::ID cipher_in)
{
  return { cipher_in };
}

template<>
const AEADCipher&
AEADCipher::get<AEAD::ID::AES_128_GCM>()
{
  static const auto instance = make_aead(AEAD::ID::AES_128_GCM);
  return instance;
}

// XXX We can't actually instantiate these ciphers right now
#if 0
template<>
const AEADCipher&
AEADCipher::get<AEAD::ID::AES_256_GCM>()
{
  static const auto instance = make_aead(AEAD::ID::AES_256_GCM);
  return instance;
}

template<>
const AEADCipher&
AEADCipher::get<AEAD::ID::CHACHA20_POLY1305>()
{
  static const auto instance = make_aead(AEAD::ID::CHACHA20_POLY1305);
  return instance;
}
#endif

static size_t
cipher_key_size(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
      return 16;

    case AEAD::ID::AES_256_GCM:
    case AEAD::ID::CHACHA20_POLY1305:
      return 32;

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}

static size_t
cipher_nonce_size(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
    case AEAD::ID::AES_256_GCM:
    case AEAD::ID::CHACHA20_POLY1305:
      return 12;

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}

static size_t
cipher_tag_size(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
    case AEAD::ID::AES_256_GCM:
    case AEAD::ID::CHACHA20_POLY1305:
      return 16;

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}

AEADCipher::AEADCipher(AEAD::ID id_in)
  : AEAD(id_in, cipher_key_size(id_in), cipher_nonce_size(id_in))
  , tag_size(cipher_tag_size(id))
{
}

bytes
AEADCipher::seal(const bytes& key,
                 const bytes& nonce,
                 const bytes& aad,
                 const bytes& pt) const
{
  auto ct = bytes(pt.size() + tag_size);

  const auto rv = cmox_aead_encrypt(CMOX_AESSMALL_GCMSMALL_ENC_ALGO,
                                    pt.data(),
                                    pt.size(),
                                    tag_size,
                                    key.data(),
                                    CMOX_CIPHER_128_BIT_KEY,
                                    nonce.data(),
                                    nonce.size(),
                                    aad.data(),
                                    aad.size(),
                                    ct.data(),
                                    nullptr);

  if (rv != CMOX_CIPHER_SUCCESS) {
    throw CMOXError::from_code(rv);
  }

  return ct;
}

std::optional<bytes>
AEADCipher::open(const bytes& key,
                 const bytes& nonce,
                 const bytes& aad,
                 const bytes& ct) const
{
  if (ct.size() < tag_size) {
    throw std::runtime_error("AEAD ciphertext smaller than tag size");
  }

  auto pt = bytes(ct.size() - tag_size);
  auto pt_size = pt.size();
  const auto rv = cmox_aead_decrypt(CMOX_AESSMALL_GCMSMALL_ENC_ALGO,
                                    ct.data(),
                                    ct.size(),
                                    tag_size,
                                    key.data(),
                                    CMOX_CIPHER_128_BIT_KEY,
                                    nonce.data(),
                                    nonce.size(),
                                    aad.data(),
                                    aad.size(),
                                    pt.data(),
                                    &pt_size);

  if (rv != CMOX_CIPHER_SUCCESS && rv != CMOX_CIPHER_AUTH_SUCCESS) {
    throw CMOXError::from_code(rv);
  }

  return pt;
}

} // namespace MLS_NAMESPACE::hpke
