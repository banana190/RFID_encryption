#ifndef ECDH
#define ECDH

#include "mbedtls/ecdh.h"
int ecdh_key_exchange(mbedtls_ecdh_context *ctx,const char *Bob_public_key_base64, size_t Bob_public_key_len);
int ecdh_public_key_generate(mbedtls_ecdh_context *ctx,uint8_t *public_key, size_t *public_key_len);
#endif