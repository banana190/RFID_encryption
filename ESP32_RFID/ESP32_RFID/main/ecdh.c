#include <stdio.h>
#include "string.h"
#include "wifi_connect.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"
#include "mbedtls/ecp.h"
#include "mbedtls/md.h"
#include "mbedtls/hkdf.h"
#include "esp_log.h"


int ECDH_derive_aes_key(const unsigned char *shared_key, size_t secret_key_len,
                   const unsigned char *info, size_t info_len,
                   unsigned char *aes_key, size_t aes_key_len) {
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    // server must have same salt
    const unsigned char salt = "pepper"; // this is pepper/0

    // mbedtls_hkdf need to enable in SDK configuration.
    int ret = mbedtls_hkdf(md_info, salt, strlen(salt),   
                           shared_key, secret_key_len,
                           info, info_len,
                           aes_key, aes_key_len);
    return ret;
}

int ecdh_public_key_generate(mbedtls_ecdh_context *ctx,uint8_t *public_key, size_t *public_key_len)
{
    mbedtls_ecdh_init(ctx); 
    mbedtls_ecdh_setup(ctx, MBEDTLS_ECP_DP_SECP256R1); // we use 

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    const char *personalization = "Your Personalization String";
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                          (const unsigned char *) personalization, strlen(personalization));

    size_t olen;
    int ret = mbedtls_ecdh_make_public(ctx, &olen, public_key, MBEDTLS_ECP_MAX_PT_LEN, mbedtls_ctr_drbg_random, &ctr_drbg);

    *public_key_len = olen;
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return ret;
}
int ecdh_key_exchange(mbedtls_ecdh_context *ctx,const char *Bob_public_key_base64, size_t Bob_public_key_len) {


    mbedtls_ctr_drbg_context ctr_drbg; // RNG generater
    mbedtls_ctr_drbg_init(&ctr_drbg); 

    mbedtls_entropy_context entropy; //entropy for RNG
    mbedtls_entropy_init(&entropy);

    

    const char *personalization = "_yiyi : maybe I'll change the RNG generater to TRNG on esp32";
    //seed generation
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *) personalization,
                                strlen(personalization));
    // ret is 0 if successful
    if (ret != 0) {
        // TODO: error handle
    }
    size_t olen;
    unsigned char buffer[512]; 

    // publuc key is x-coordinate and y-coordinate 
    ret = mbedtls_ecdh_make_public(ctx, &olen, buffer, sizeof(buffer), mbedtls_ctr_drbg_random, &ctr_drbg);
    
    // ret is 0 if successful
    if (ret != 0) {
        // TODO: error handle
    }

    // example of Bob's public key (base64) ,need to fetch from server
    // Bob's private key : 03975b201048d7b7f1c9cb9e821c7ce4e07ccd35204f81628571e0b1be8c6b02 (hex)
    // char* Bob_public_key_base64  = "BBgebuTPXePNepujLdW1aHSUHAiofJ2bX9UAqcpjBcc04A3WobQpni7bPHW1b6lvXajbg7rLCyniRKLif7BGlh8=";
    unsigned char Bob_public_key[MBEDTLS_ECP_MAX_PT_LEN];
    size_t len;
    ret = mbedtls_base64_decode(Bob_public_key, Bob_public_key_len,
                                    &len, (const unsigned char*)Bob_public_key_base64,
                                    strlen(Bob_public_key_base64));
    
    // handle the public key
    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

    mbedtls_ecp_point Bob_ecp_point;
    mbedtls_ecp_point_init(&Bob_ecp_point);
    ret = mbedtls_ecp_point_read_binary(&grp, &Bob_ecp_point, Bob_public_key, len);
    if (ret != 0) {
        // error handling
    }    

    // calculate the shared key
    unsigned char shared_secret[MBEDTLS_ECP_MAX_BYTES];
    ret = mbedtls_ecdh_calc_secret(ctx, &olen, shared_secret, sizeof(shared_secret),
                                        mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        // error handling
    }

    //AES setup
    unsigned char aes_key[32];
    size_t aes_key_len = sizeof(aes_key);
    // TODO: KDF(Key derivation function)? DONE!
    // memcpy(aes_key, shared_secret, 32); // just cut it D:
    const char *info = "yiyi"; // server must have the same info 
    size_t info_len = strlen(info);
    ret = ECDH_derive_aes_key(shared_secret, olen, (unsigned char *)info, info_len, aes_key, aes_key_len);
    if (ret == 0) {
        ESP_LOGI("ECDH","AES key derived successfully");
    } else {
        ESP_LOGE("ECDH","Failed to derive AES key");
    }

    // here is to write the AES key into the RFID card

    
    
    


    //free memory
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_ecp_point_free(&Bob_ecp_point);
    mbedtls_ecp_group_free(&grp);
    mbedtls_mpi_free(&shared_secret);

    return ret;
}