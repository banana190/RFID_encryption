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

#include "esp_flash.h"



#define PORT 9527
#define CHAR_SET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!~@#$^()_+=-"
#define Signiture_length 16
#define FLASH_ADDR 0x10000 // this is the RSA private key address
#define AES_KEY_SIZE 32



static const char *TAGg = "example";

// pkcs7 is that if the length is 31 mod 16 = 15 then you fill 16-15 = 0x01 one times  
//                                40 mod 16 = 8  then you fill 16- 8 = 0x08 eight times           
size_t pad_string_pkcs7(char *input_string, size_t *length) {
    ESP_LOGI("pkcs7", "length before doing any process for padding : %zu", *length);
    int block_size = 16; // for AES the input block size is 16 bytes (128 bits)
    int input_length = strlen(input_string); 
    int padding_size = block_size - (input_length % block_size); // 16 - length mod 16
    size_t temp = *length + padding_size;
    if (padding_size == 0) {
        return *length;          // the size if perfect no need to padding
    }
    char padding = (char)padding_size;
    for (int i = 0; i < padding_size; i++) {
        input_string[input_length + i] = padding;
    }
    // Since I did't put \0 at the end of the string(will make it become 16x + 1), 
    // this length will search until find \0.
    // I thought the compiler would add \0 automatically but it doesn't... 
    // so it cause stack overflow here since input wrong length.
    // btw I don't know will the \0 effect my AES encryption or not.
    // ESP_LOGI("pkcs7", "length before padding : %zu", *length);
    // ESP_LOGI("pkcs7", "padding size : %d", padding_size);
    // *length += padding_size; 
    ESP_LOGI("pkcs7", "length after padding : %zu", temp);
    ESP_LOGI("pkcs7", "padding done!");
    return temp;
}

void AES_encrypter(const uint8_t *data, size_t data_len, const uint8_t *aes_key,uint8_t *encrypted_data) // haven't test yet hope will work
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int ret = mbedtls_aes_setkey_enc(&aes, aes_key, AES_KEY_SIZE * 8);
    if(ret != 0)
    {
        ESP_LOGE("AES","AES key set error");
        return;
    }
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, data, encrypted_data);
    mbedtls_aes_free(&aes);
}

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
    // const char *message = "This is ESP32 broadcasting"; // TODO: This message should be encrypted
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

int HMAC_derive_AES_for_RSA_privateKey(unsigned char *derived_key) // this will always generate same AES key for RSA
{
    const char *salt = "Please stop stack overflowing thank you please god"; // I will change this in real application
    const int iterations = 1000;   // I will change this in real application
    int ret;
    if (AES_KEY_SIZE!=32)
    {
        ret = -1;
        ESP_LOGE("AES","Output size error! Key output size must be 32 bytes");
    }

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

    return ret;
}

void flash_writer() // this function might only be used for once since after I encrypted the RSA into flash
{
    const esp_partition_t* rsa_key_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, "RSA_key");
    if (!rsa_key_partition) {
        ESP_LOGE("Flash Writer", "RSA_key partition not found");
        return;
    }   
    unsigned char AES_key[AES_KEY_SIZE]; // AES-256 key size must be 32 bytes.
    if (HMAC_derive_AES_for_RSA_privateKey(AES_key)!=0){
        return;
    }
    char RSA_private_key[] = "RSA private_key"; // I'm using RSA-2048 private key 
    ESP_LOGI("Flash Writer", " RSA private key loaded successfully");
    size_t RSA_private_key_length = strlen(RSA_private_key);
    ESP_LOGI("RSA","Private key length: %d", RSA_private_key_length); 
    uint8_t* RSA_encrypted_key = malloc(4096); // I cut a 4kb flash to store the encrypted RSA key
    memset(RSA_encrypted_key, 0, 4096);
    RSA_private_key_length = pad_string_pkcs7(RSA_private_key,&RSA_private_key_length);
    ESP_LOGI("RSA","Private key length arter padding: %d", RSA_private_key_length);
    for (int i = 0; i < RSA_private_key_length/16; i++) {
        unsigned char temp[16]; // AES input 16 bytes
        for (int j = 0; j < 16; j++) {
            temp[j]= RSA_private_key[i*16 + j];
        }
        AES_encrypter(temp, sizeof(temp), AES_key, temp);
        for (int j = 0; j < 16; j++) {
            RSA_encrypted_key[i * 16 + j] = temp[j];
        }        
        ESP_LOGI("AES", "AES encrypting : block%d",i); 
    }
    ESP_LOGI("AES", "AES encryption done");
    ESP_LOGI("RSA", "RSA encrypted key:");
    // for (int i = 0; i < sizeof(RSA_private_key); i++) {
    //     printf("%c", RSA_encrypted_key[i]);
    // }
    // printf("\n"); // this just for checking is there anything in RSA_encrypted_key[]
    ESP_LOGI("Flash", "label: %s",rsa_key_partition->label);
    ESP_LOGI("Flash", "Erase_size: %"PRIu32"",rsa_key_partition->erase_size);
    ESP_LOGI("Flash", "Size: %"PRIu32"",rsa_key_partition->size);
    ESP_LOGI("Flash", "Address: %"PRIu32"",rsa_key_partition->address);
    // I realize that esp_idf will erase the flash by default so it doesn't really need to erase it.
    // esp_err_t ret = esp_partition_erase_range(rsa_key_partition, 0, 4096);
    // if (ret != ESP_OK) {
    // ESP_LOGE("AES", "Failed to erase partition: %s", esp_err_to_name(ret));
    // return;
    // }
    // esp_flash_init(NULL); //<- if NULL then don't need to init (#10516)
    // ESP_LOGI("Flash Writer", "Initializing flash");
    esp_err_t ret = esp_flash_write(NULL,RSA_encrypted_key, rsa_key_partition->address, 4096);
    if (ret != ESP_OK) {
        ESP_LOGE("Flash Writer", "Error writing data to flash: %d", ret);
        return;
    }
    ESP_LOGI("Flash Writer", "Flash done: %d", ret);
    free(RSA_encrypted_key);
}


