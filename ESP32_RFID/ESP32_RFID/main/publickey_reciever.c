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
#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"
#include "mbedtls/aes.h"


#define PORT 9527
#define CHAR_SET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!~@#$^()_+=-"
#define Signiture_length 16
#define FLASH_ADDR 0x10000 // this is the RSA private key address
#define AES_KEY_SIZE 32



static const char *TAGg = "example";

char* generate_random_string(int length) {
    char* random_string = malloc((length + 1) * sizeof(char));
    if (random_string == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < length; i++) {
        int random_index = esp_random() % (strlen(CHAR_SET));
        random_string[i] = CHAR_SET[random_index];
    }
    random_string[length] = '\0';
    
    return random_string;
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
}

char RSA_Sign_with_SHA256()//WIP
{
    char* signature = generate_random_string(Signiture_length);
    return signature;
}

char HMAC_derive_AES_for_RSA_privateKey() // this will always generate same AES key for RSA
{
    const char *salt = "Please stop stack overflowing thank you please god"; // I will change this in real application
    const int iterations = 1000;   // I will change this in real application
    unsigned char derived_key[32]; // AES-256
    int ret;

    ret = mbedtls_pkcs5_pbkdf2_hmac_ext(
        MBEDTLS_MD_SHA256,  // SHA-256 as hash
        NULL, 0,            // get key from efuse
        (const unsigned char *)salt, strlen(salt), // add salt
        iterations,
        sizeof(derived_key) * 8, 
        derived_key
    );

    if (ret == 0) {
        ESP_LOGI("AES Key Derivation", "Key derived successfully.");
    } else {
        ESP_LOGE("AES Key Derivation", "Failed to derive key: %d", ret);
    }

    return derived_key;
}

void flash_writer() // this function might only be used for once since after I encrypted the RSA into flash
{
    char RSA_private_key[700]; // I'm using RSA-2048 so the private key is like 600-700 bytes long.
    uint8_t encrypt_key[16]    // I wish it can be 16 bytes long... wish.
    = AES_encrypter(RSA_private_key, sizeof(RSA_private_key), HMAC_derive_AES_for_RSA_privateKey()); // I'm gonna sleep so the size...yeah
    int ret = spi_flash_write(FLASH_ADDR, encrypt_key, 16);
    if (ret != ESP_OK) {
        ESP_LOGE("Flash Writer", "Error writing data to flash: %d", ret);
        return;
    }
}

uint8_t AES_encrypter(const uint8_t *data, size_t data_len, const uint8_t *aes_key) // haven't test yet hope will work
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int ret = mbedtls_aes_setkey_enc(&aes, aes_key, AES_KEY_SIZE * 8);
    if(ret != 0)
    {
        ESP_LOGE("AES","AES key set error");
        return;
    }

    uint8_t encrypted_data[data_len]; // the size might need to be change
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, data, encrypted_data);
    mbedtls_aes_free(&aes);
    return encrypted_data;
}