#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include "esp_flash.h"
#include "esp_ds.h"

#include "flash_write_rsa_key.h"



#define PORT 9527
#define Signiture_length 16
#define FLASH_ADDR 0x10000 // this is the RSA private key address
#define AES_KEY_SIZE 32



static const char *TAGg = "example";

int key_pair_checker() {
    int ret;
    const char *private_key_pem = TESTING_RSA_PRIVATE_KEY;
    const char *public_key_pem = TESTING_RSA_PUBLIC_KEY;
    mbedtls_pk_context pk_context;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char hash[32];
    unsigned char buf[512];
    size_t olen = 0;

    mbedtls_pk_init(&pk_context);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
        printf("Failed in mbedtls_ctr_drbg_seed: %d\n", ret);
        return 1;
    }

    if ((ret = mbedtls_pk_parse_key(&pk_context, (const unsigned char *)private_key_pem, strlen(private_key_pem) + 1,NULL,0, mbedtls_ctr_drbg_seed, &ctr_drbg)) != 0) {
        printf("Failed loading private key: %d\n", ret);
        return 1;
    }

    
    const char *message = "Hello, world!";
    memcpy(hash, message, strlen(message)); 
    if ((ret = mbedtls_pk_sign(&pk_context, MBEDTLS_MD_SHA256, hash, 0, buf,sizeof(buf), &olen, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
        printf("Failed to sign: %d\n", ret);
        return 1;
    }

    mbedtls_pk_free(&pk_context);
    mbedtls_pk_init(&pk_context);
    if ((ret = mbedtls_pk_parse_public_key(&pk_context, (const unsigned char *) public_key_pem, strlen(public_key_pem) + 1)) != 0) {
        ESP_LOGI("RSA key pair checker","Failed loading public key: %d\n", ret);
        return 1;
    }

    if ((ret = mbedtls_pk_verify(&pk_context, MBEDTLS_MD_SHA256, hash, 0, buf, olen)) != 0) {
        ESP_LOGI("RSA key pair checker","Failed to verify signature: %d\n", ret);
        return 1;
    }

    ESP_LOGI("RSA key pair checker","Signature verified successfully\n");

    mbedtls_pk_free(&pk_context);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return 0;
}

int verify_signature(
    unsigned char *message, size_t message_len,          
    unsigned char *signature, size_t signature_len       
) {
    ESP_LOGI("RSA signature verification","input message");
    print_hex_uint8_t(message, message_len);
    ESP_LOGI("RSA signature verification","input signature");
    print_hex_uint8_t(signature, signature_len);
    int ret;
    mbedtls_pk_context pk_context;
    mbedtls_pk_init(&pk_context);

    const char *public_key =TESTING_RSA_PUBLIC_KEY;
    ESP_LOGI("RSA signature verification","public key loaded successfully");
    ret = mbedtls_pk_parse_public_key(&pk_context, (const unsigned char *) public_key, strlen(public_key)+1);
    if (ret != 0) {
        ESP_LOGE("RSA signature verification","public_key parse failed");
        mbedtls_pk_free(&pk_context);
        return ret;
    }
    ESP_LOGI("RSA signature verification","public key parse successfully");
    if (mbedtls_pk_get_type(&pk_context) != MBEDTLS_PK_RSA) {
        ESP_LOGE("RSA signature verification","public_key get type failed");
        mbedtls_pk_free(&pk_context);
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }
    unsigned char hash[32];
    mbedtls_sha256(message, message_len, hash, 0);

    ret = mbedtls_pk_verify(&pk_context, MBEDTLS_MD_SHA256, hash, 32, signature, signature_len);

    mbedtls_pk_free(&pk_context);

    return ret;
}



void udp_server_task(void *pvParameters) {
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE("UDP", "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI("UDP", "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE("UDP", "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI("UDP", "Socket bound, port %d", PORT);

        while (1) {

            ESP_LOGI("UDP", "Waiting for data");
            struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE("UDP", "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's IP address as string
                if (source_addr.sin_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI("UDP", "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI("UDP", "%s", rx_buffer);

                const char *tx_buffer = "Hello from ESP32"; //TODO: change the message to RSA
                source_addr.sin_port = htons(PORT);
                for (int i = 0; i < 2; i++) {
                int err = sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                ESP_LOGI("UDP", "Message sent to client: %s", tx_buffer);
                if (err < 0) {
                    ESP_LOGE("UDP", "Error occurred during sending: errno %d", errno);
                }
                
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE("UDP", "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}


void udp_broadcaster()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE("UDP", "Failed to create socket");
        vTaskDelay(portMAX_DELAY);
    }

    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ESP_LOGE("UPD", "Failed to set broadcast option");
        close(sock);
        vTaskDelay(portMAX_DELAY);
    }
    const char *message = "This is ESP32 broadcasting"; // TODO: This message should be encrypted
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr);
    int ret = sendto(sock, message, strlen(message), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret < 0) {
        ESP_LOGE("UDP", "Failed to send broadcast message");
    } else {
        ESP_LOGI("UDP", "Broadcast message sent");
    }
    close(sock);
}


int mbedtls_rsa_sign(uint8_t *data, size_t data_len, uint8_t *private_key, size_t private_key_len, uint8_t *signature, size_t *signature_len) {
    ESP_LOGI("RSA","RSA signature space: %zu",*signature_len);

    int ret = 1;
    mbedtls_pk_context ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    
    
    mbedtls_pk_init(&ctx);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    const char *pers = "The personalization string. \
                        This can be NULL, in which case the personalization string \
                        is empty regardless of the value of len.";
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    // ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, MBEDTLS_CTR_DRBG_ENTROPY_LEN);
    if (ret != 0) {
        ESP_LOGE("RSA Sign", "mbedtls_ctr_drbg_seed error");
        ESP_ERROR_CHECK(ret);
        goto cleanup;
    }
    ESP_LOGI("RSA Sign", "mbedtls_ctr_drbg_seed set success");


    // this function analyzes the RSA private key and store it to ctx .
    // note:
    //      int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t output_len)
    //      int                (*f_rng)(void *     , unsigned char *      , size_t)
    ret = mbedtls_pk_parse_key(&ctx, (const unsigned char *)private_key, private_key_len+1, NULL, 0,mbedtls_ctr_drbg_random,&ctr_drbg);
    if (ret != 0) {
        ESP_LOGE("RSA Sign", "mbedtls_pk_parse_key error");
        printf("Error code: %d\n", ret);
        ESP_ERROR_CHECK(ret);
        goto cleanup;
    }
    ESP_LOGI("RSA Sign", "mbedtls_pk_parse_key set success");

    // mbedtls_rsa_context rsa;
    // mbedtls_rsa_context *temp_rsa = mbedtls_pk_rsa(ctx);
    // memcpy(&rsa,temp_rsa,sizeof(mbedtls_rsa_context));

    // VERY IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // mbedtls_pk_sign cannot feel anything about this hash is 32 bytes long or not
    // which means even a "A" can be input, which must be invalid since the SHA256 always generate same length
    // mbedtls_pk_sign should base on mbedtls_md_type_t to check the hash length
    // I try to fix the wrong signature and looking everywhere like 10hrs and realize this...
    unsigned char hash[32];
    mbedtls_sha256(data, data_len, hash, 0);

    // this function creates a signature         // this 32 is come from SHA256, the hash length is 32 bytes.
    ret = mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA256, hash, 0, signature,*signature_len, signature_len, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        ESP_LOGE("RSA Sign", "mbedtls_pk_sign error");
        printf("Error code: %d\n", ret);
        ESP_ERROR_CHECK(ret);
        goto cleanup;
    }
    ESP_LOGI("RSA Sign", "mbedtls_pk_sign success");


    ret = 0;

cleanup:
    mbedtls_pk_free(&ctx);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
    // return rsa;
}

void unnamed()
{
    key_pair_checker();
    uint8_t data_to_sign[] = "A"; // this will be a random string but now it's just testing.
    // uint8_t data_to_sign[] = "\x41"; 
    ESP_LOGI("RSA sign Testing","input string: %u", data_to_sign[0]);
    size_t data_len = sizeof(data_to_sign);
    ESP_LOGI("RSA sign Testing","data_to_sign loaded");
    uint8_t signature[256]; // Adjust size based on expected signature length
    size_t signature_len = sizeof(signature);
    ESP_LOGI("RSA sign Testing","singature space loaded: %zu bytes", signature_len/8);
    uint8_t *decrypted_rsa_private_key = malloc(4096); // Ensure your buffer is appropriately sized
    size_t decrypted_rsa_private_key_len = flash_reader(decrypted_rsa_private_key, 4096);
    ESP_LOGI("RSA sign Testing","decrypted RSA private key loaded from flash");
    print_hex_uint8_t(decrypted_rsa_private_key, decrypted_rsa_private_key_len);

    // unsigned char testing_rsa_private_key_pem[] = TESTING_RSA_PRIVATE_KEY;
    // size_t testing_rsa_private_key_pem_length = strlen((const char*)testing_rsa_private_key_pem);

    int ret = mbedtls_rsa_sign(data_to_sign, data_len, decrypted_rsa_private_key, decrypted_rsa_private_key_len, signature, &signature_len);
    // mbedtls_rsa_context rsa_ctx = mbedtls_rsa_sign(data_to_sign, data_len, decrypted_rsa_private_key, decrypted_rsa_private_key_len, signature, &signature_len);
    ESP_LOGI("RSA sign Testing","Signature:");
    // IDK why this signature is wrong... , it still 32 bytes but different compared to other signatures generater.
    // After 5 hours I've try A ,A\x00 ,A\0   [0x41,0x00] "\x41\x00" ... etc 
    // but everything doesn't match to this signature.
    // JavaScript Python Golang... everyone generates same signature but this uhhh thing is just different.
    print_hex_uint8_t(signature, signature_len);
    ret = verify_signature(data_to_sign, data_len, signature,signature_len);
    if (ret != 0)
    {
        ESP_LOGE("RSA verify Testing","The signature isn't match!!");
        ESP_LOGI("RSA verify Testing","If you see signature isn't match, go to flash_write_rsa_key.h change the define");
    }
    else 
        ESP_LOGI("RSA verify Testing","The signature matched");

    free(decrypted_rsa_private_key);



}

