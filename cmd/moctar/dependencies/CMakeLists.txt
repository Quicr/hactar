add_subdirectory(libquicr)

set(OPENSSL_FOUND ON)
set(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libquicr/dependencies/transport/dependencies/boringssl)
set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
set(OPENSSL_CRYPTO_LIBRARY ${CMAKE_BINARY_DIR}/dependencies/libquicr/dependencies/transport/dependencies/boringssl/crypto/libcrypto.a)
set(OPENSSL_SSL_LIBRARY ${CMAKE_BINARY_DIR}/dependencies/libquicr/dependencies/transport/dependencies/boringssl/ssl)
add_subdirectory(mlspp)
