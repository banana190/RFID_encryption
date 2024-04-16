#include <stdio.h>
#include "string.h"
#include "wifi_connect.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"
#include "mbedtls/ecp.h"
#include "mbedtls/md.h"
#include "esp_log.h"
#include "ecdh.h" 

void ecdh_key_exchange() {


    mbedtls_ecdh_context ctx; // context for ECDH handshake, private key is in this context
    mbedtls_ecdh_init(&ctx); 
    mbedtls_ecdh_setup(&ctx, MBEDTLS_ECP_DP_SECP256R1); // we use 

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
    ret = mbedtls_ecdh_make_public(&ctx, &olen, buffer, sizeof(buffer), mbedtls_ctr_drbg_random, &ctr_drbg);
    
    // ret is 0 if successful
    if (ret != 0) {
        // TODO: error handle
    }

    // example of Bob's public key (base64) ,need to fetch from server
    // Bob's private key : 03975b201048d7b7f1c9cb9e821c7ce4e07ccd35204f81628571e0b1be8c6b02 (hex)
    char* Bob_public_key_base64  = "BBgebuTPXePNepujLdW1aHSUHAiofJ2bX9UAqcpjBcc04A3WobQpni7bPHW1b6lvXajbg7rLCyniRKLif7BGlh8=";
    unsigned char Bob_public_key[MBEDTLS_ECP_MAX_PT_LEN];
    size_t len;
    ret = mbedtls_base64_decode(Bob_public_key, sizeof(Bob_public_key),
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
    ret = mbedtls_ecdh_calc_secret(&ctx, &olen, shared_secret, sizeof(shared_secret),
                                        mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        // error handling
    }

    //AES setup
    unsigned char aes_key[32];
    // TODO: KDF(Key derivation function)?
    memcpy(aes_key, shared_secret, 32); // just cut it D:
    
    
    


    //free memory
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_ecp_point_free(&Bob_ecp_point);
    mbedtls_ecp_group_free(&grp);
    mbedtls_mpi_free(&shared_secret);


}