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

int dummy_rng(void *p_rng, unsigned char *output, size_t output_size) {
    memset(output, 0, output_size);
    return 0;
    // 0x4480    RSA - The random generator failed to generate non-zeros
    // really mad
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


mbedtls_rsa_context mbedtls_rsa_sign(uint8_t *data, size_t data_len, uint8_t *private_key, size_t private_key_len, uint8_t *signature, size_t *signature_len) {
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
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
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
    ret = mbedtls_pk_parse_key(&ctx, private_key, private_key_len, NULL, 0,mbedtls_ctr_drbg_random,&ctr_drbg);
    if (ret != 0) {
        ESP_LOGE("RSA Sign", "mbedtls_pk_parse_key error");
        printf("Error code: %d\n", ret);
        ESP_ERROR_CHECK(ret);
        goto cleanup;
    }
    ESP_LOGI("RSA Sign", "mbedtls_pk_parse_key set success");

    mbedtls_rsa_context *rsa = mbedtls_pk_rsa(ctx);


    // this function creates a signature         // this 32 is come from SHA256, the hash length is 32 bytes.
    ret = mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA256, data, 32, signature,*signature_len, signature_len, mbedtls_ctr_drbg_random, &ctr_drbg);
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

    return *rsa;
}

void unnamed()
{
    // uint8_t data_to_sign[] = "A"; // this will be a random string but now it's just testing.
    uint8_t data_to_sign[] = "\x41"; 
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
    mbedtls_rsa_context rsa_ctx = mbedtls_rsa_sign(data_to_sign, data_len, decrypted_rsa_private_key, decrypted_rsa_private_key_len, signature, &signature_len);
    ESP_LOGI("RSA sign Testing","Signature:");
    // IDK why this signature is wrong... , it still 32 bytes but different compared to other signatures generater.
    // After 5 hours I've try A ,A\x00 ,A\0   [0x41,0x00] "\x41\x00" ... etc 
    // but everything doesn't match to this signature.
    // JavaScript Python Golang... everyone generates same signature but this uhhh thing is just different.
    print_hex_uint8_t(signature, signature_len);
    // Trying esp_ds hope it will work.
    
    esp_ds_data_t ds_data;
    ds_data.rsa_length = ESP_DS_RSA_2048;
    uint8_t iv[16];
    esp_fill_random(iv, sizeof(iv));
    memcpy(ds_data.iv, iv, sizeof(iv));
    
    uint8_t rsa_params[]="testing";
    uint8_t aes_key[32]; 
    size_t aes_key_len = sizeof(aes_key);

    esp_ds_p_data_t ds_p_data;
    mbedtls_mpi *modulus = &rsa_ctx.MBEDTLS_PRIVATE(N);
    mbedtls_mpi *exponent = &rsa_ctx.MBEDTLS_PRIVATE(E);
    // I have no idea what is the point correct or not but I'm gonna sleep :D
    // Like I free the ctx so the rsa_ctx must explode but ^^^^^^^^^^^^^^^^^^
    // will be fixed tmrw , Clueless.
    mbedtls_mpi_read_binary(modulus,*ds_p_data.M,256);
    mbedtls_mpi_read_binary(exponent,*ds_p_data.Y,256);


    free(decrypted_rsa_private_key);
}

