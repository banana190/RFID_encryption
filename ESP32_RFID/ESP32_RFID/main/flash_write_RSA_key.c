#include <stdio.h>
#include <string.h>
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mbedtls/sha256.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"
#include "mbedtls/aes.h"

#define AES_KEY_SIZE 32
#define Input_RSA_private_key  "your key here"
// based on partiton.csv file.
// e.g. RSA_key,data,nvs_keys,,0x1000 <--- the size is 4kb so it's 4096
#define PARTITION_SIZE 4096 

void AES_encrypter(const uint8_t *data, size_t data_len, const uint8_t *aes_key,uint8_t *encrypted_data)
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

void AES_decrypter(const uint8_t *encrypted_data, size_t data_len, const uint8_t *aes_key, uint8_t *decrypted_data)
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int ret = mbedtls_aes_setkey_dec(&aes, aes_key, AES_KEY_SIZE * 8); 
    if(ret != 0)
    {
        ESP_LOGE("AES", "AES key set error");
        return;
    }
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encrypted_data, decrypted_data);
    mbedtls_aes_free(&aes);
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

void print_hex_uint8_t(uint8_t *str, size_t size) {
    for (int i = 0; i < size; i++) {
        printf("%02X ", str[i]);
    }
    printf("\n");
}

// pkcs7 is that if the length is 31 mod 16 = 15 then you fill 16-15 = 0x01 one times  
//                                40 mod 16 = 8  then you fill 16- 8 = 0x08 eight times  
void pad_string_pkcs7(uint8_t *input_string, size_t *length) {
    ESP_LOGI("pkcs7", "length before doing any process for padding : %zu", *length);
    print_hex_uint8_t(input_string, *length);
    int block_size = 16; // for AES the input block size is 16 bytes (128 bits)
    // int input_length = strlen(input_string); 
    int padding_size = block_size - (*length % block_size); // 16 - length mod 16
    int target_length = *length + padding_size;
    ESP_LOGI("pkcs7", "padding size : %d",padding_size);
    size_t temp;
    if (padding_size == 16) {
        return;          // the size if perfect no need to padding
    }
    char padding = (char)padding_size;
    ESP_LOGI("pkcs7", "padding start");
    for (temp = *length; temp < target_length; temp++) {
        ESP_LOGI("pkcs7", "padding at: %zu",temp);
        // ESP_LOGI("pkcs7", "length in padding : %zu", *length);
        input_string[temp] = padding;
    }
    // Since I did't put \0 at the end of the string(will make it become 16x + 1), 
    // this length will search until find \0.
    // I thought the compiler would add \0 automatically but it doesn't... 
    // so it cause stack overflow here since input wrong length.
    // btw I don't know will the \0 effect my AES encryption or not.
    // ESP_LOGI("pkcs7", "length before padding : %zu", *length);
    // ESP_LOGI("pkcs7", "padding size : %d", padding_size);
    // *length += padding_size; 

    ESP_LOGI("pkcs7", "padding done! String after padding :");
    print_hex_uint8_t(input_string, temp); // I use hex because %c cannot work with ASCII 1~31
    *length = temp;
    ESP_LOGI("pkcs7", "length after padding : %zu", *length);
}

void depad_string_pkcs7(uint8_t *input_string, size_t *length)
{
    ESP_LOGI("pkcs7", "length before doing any process for depadding : %zu", *length);
    print_hex_uint8_t(input_string, *length);
    if (*length %16 !=0)
    {
        ESP_LOGE("pkcs7","The input size is not corrected! The encrypted data's length should be 16n");
        return;
    }
    uint8_t padding_length = input_string[*length - 1];
    ESP_LOGI("pkcs7", "padding length : %u", padding_length);
    if (padding_length >17)
    {
        ESP_LOGE("pkcs7","The padding patten is not corrected! We used pcks7");
        return;
    }
    *length -= padding_length;
    input_string[*length] = "\0";

    ESP_LOGI("pkcs7","depadding done, the length now is: %zu",*length);
}

/**
 * @brief Encrypts or decrypts a string using AES-256.
 *
 *
 * @param input_string  your input string should be dynamically allocated to prevent stack overflow
 * @note example: "uint8_t* input_string = malloc(SIZE)"
 * @param mode         '0' for encryption and '1' for decryption.
 *
 * @return size_t <encrypted string length>
 * @warning If the input_string is not dynamically allocated, this function may cause a stack overflow.
 */
size_t string_with_AES(uint8_t *input_string,size_t length,bool mode)
{
    ESP_LOGI("AES","input string: ");
    print_hex_uint8_t(input_string,length);
    unsigned char AES_key[AES_KEY_SIZE]; // AES-256 key size must be 32 bytes.
    if (HMAC_derive_AES_for_RSA_privateKey(AES_key)!=0){
        return 0;
    }    
    ESP_LOGI("AES","AES KEY:");
    print_hex_uint8_t(AES_key,AES_KEY_SIZE);
    ESP_LOGI("AES","Input string length: %d", length);
    if(!mode) 
        pad_string_pkcs7(input_string,&length);
    
    for (int i = 0; i < length/16; i++) {
        unsigned char temp[16]; // AES input 16 bytes
        for (int j = 0; j < 16; j++) {
            temp[j]= input_string[i*16 + j];
        }
        if(mode)
        {
            AES_decrypter(temp, sizeof(temp), AES_key, temp);
            ESP_LOGI("AES", "AES decrypting : block%d",i); 
        }
        else
        {
            AES_encrypter(temp, sizeof(temp), AES_key, temp);
            ESP_LOGI("AES", "AES encrypting : block%d",i); 
        }
        for (int j = 0; j < 16; j++) {
            input_string[i * 16 + j] = temp[j];
        }        
    }    
    ESP_LOGI("AES","AES task success");
    if(!mode)
    {
        ESP_LOGI("AES","encrypted data:");
        print_hex_uint8_t(input_string,length);
    }
    else
    {
        depad_string_pkcs7(input_string,&length);
        ESP_LOGI("AES","decrypted data:");
        print_hex_uint8_t(input_string,length);
    }
    // I realize that the length didn't update because I didn't use length*
    // But I'm too lazy to rewrite the function so I just return the length value to update it
    // maybe I'll rewrite it someday. Clueless
    return length;
}

void flash_writer() // this function might only be used for once since after I encrypted the RSA into flash
{
    const esp_partition_t* rsa_key_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, "RSA_key");
    if (!rsa_key_partition) {
        ESP_LOGE("Flash Writer", "RSA_key partition not found");
        return;
    }   
    char RSA_private_key[4096] = Input_RSA_private_key; // I'm using RSA-2048 private key 
    ESP_LOGI("Flash Writer", " RSA private key loaded successfully");
    size_t RSA_private_key_length = strlen(RSA_private_key);
    uint8_t* RSA_encrypted_key = malloc(4096); // I cut a 4kb flash to store the encrypted RSA key
    memcpy(RSA_encrypted_key, RSA_private_key, 4096);
    RSA_private_key_length = string_with_AES(RSA_encrypted_key,RSA_private_key_length,false);
    ESP_LOGI("Flash", "RSA private key encrypted and ready to flash:");
    print_hex_uint8_t(RSA_encrypted_key, RSA_private_key_length);
    ESP_LOGI("Flash", "label: %s",rsa_key_partition->label);
    ESP_LOGI("Flash", "Erase_size: %"PRIu32"",rsa_key_partition->erase_size);
    ESP_LOGI("Flash", "Size: %"PRIu32"",rsa_key_partition->size);
    ESP_LOGI("Flash", "Address: %"PRIu32"",rsa_key_partition->address);
    // I realize that esp_idf will erase the flash by default so it doesn't really need to erase it.
    // Change RSA_key,  data, nvs_keys, ,        0x1000, --->readonly<----- to keep it in flash
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

void flash_reader(uint8_t *decrypted_rsa_private_key,size_t flash_size)
{
    const esp_partition_t* rsa_key_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, "RSA_key");
    if (!rsa_key_partition) {
        ESP_LOGE("Flash Reader", "RSA_key partition not found");
        return;
    }
    uint8_t* decrypted_rsa_key = malloc(4096);
    esp_err_t ret = esp_flash_read(NULL, decrypted_rsa_key, rsa_key_partition->address, 4096);
    if (ret != ESP_OK) {
        ESP_LOGE("Flash Reader", "Error reading data from flash: %d", ret);
        free(decrypted_rsa_key);
        return;
    }
    size_t rsa_key_length = 4096;
    rsa_key_length = string_with_AES(decrypted_rsa_key, rsa_key_length, true); 
    memcpy(decrypted_rsa_private_key, decrypted_rsa_key, rsa_key_length);
    free(decrypted_rsa_key);
}

