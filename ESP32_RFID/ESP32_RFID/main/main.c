#include <stdio.h>
#include <inttypes.h>
#include "string.h"
#include "sdkconfig.h"
// freertos
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// esp and wifi
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_timer.h"

// driver
#include "driver/uart.h"
#include "driver/gpio.h"

// #include "mdns.h"
// my functions
#include "wifi_connect.h"
#include "ecdh.h" 
#include "publickey_reciever.h"
#include "flash_write_rsa_key.h"
#include "udp_communication.h"
#include "card_reader.h"
#include "tls_connection.h"
#include "lwip/apps/sntp.h"




//package for udp task
#include <sys/param.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

// package for RFID
// #include "rc522.h"
#include "PN532.h"
#define PN532_SCK 18
#define PN532_MISO 19
#define PN532_MOSI 23
#define PN532_SS 5


#define BUF_SIZE (1024)
#define PORT 9527


char hostname[20];
int UDP_maximum_listener = 0;
pn532_t nfc;
uint8_t keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//timer
int64_t start_time = 0;
int64_t end_time = 0;

// for tasks communication
extern QueueHandle_t queue;
// stop other task and wait     for encryption delay
extern SemaphoreHandle_t sem;

//

static void obtain_time()
{

    ESP_LOGI("SNTP", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    ESP_LOGI("SNTP", "Current time: %s", asctime(&timeinfo));
}


void nfc_task(void *pvParameter)
{
    pn532_spi_init(&nfc, PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
    pn532_begin(&nfc);

    uint32_t versiondata = pn532_getFirmwareVersion(&nfc);
    if (!versiondata)
    {
        ESP_LOGI("PN532", "Didn't find PN53x board");
        while (1)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    // Got ok data, print it out!
    ESP_LOGI("PN532", "Found chip PN5 %lx", (versiondata >> 24) & 0xFF);
    ESP_LOGI("PN532", "Firmware ver. %ld.%ld", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);

    // configure board to read RFID tags
    pn532_SAMConfig(&nfc);

    ESP_LOGI("PN532", "Waiting for an ISO14443A Card ...");

    while (1)
    {
        uint8_t success;
        uint8_t uidLength;
        uint8_t key_RW;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        uint8_t card_key_1[16] = {0}; 
        uint8_t card_key_2[16] = {0};
        
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
        uint8_t *card_key;

        // card_key = (uint8_t *)malloc(30 * sizeof(uint8_t));
        card_key = (uint8_t *)malloc(15 * sizeof(uint8_t));
        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uidLength will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        success = pn532_readPassiveTargetID(&nfc, PN532_MIFARE_ISO14443A, uid, &uidLength, 0);

        if (success)
        {
            start_time = esp_timer_get_time();
            // Display some basic information about the card
            ESP_LOGI("PN532", "Found an ISO14443A card");
            ESP_LOGI("PN532", "UID Length: %d bytes", uidLength);
            ESP_LOGI("PN532", "UID Value:");
            esp_log_buffer_hexdump_internal("PN532", uid, uidLength, ESP_LOG_INFO); 
            // vTaskDelay(1000 / portTICK_PERIOD_MS);         
        }
        else
        {
            // PN532 probably timed out waiting for a card
            ESP_LOGI("PN532", "Timed out waiting for a card");
        }
        ESP_LOGI("PN532", "Reading block 4");
        pn532_mifareclassic_AuthenticateBlock(&nfc, uid, uidLength, 4, 0, keyA);
        // This is odd that the first byte need to be 00 and will read a random byte from somewhere idk.
        // So instead of generating an AES key and writing to the block
        // I'm using TRNG to generate 30 bytes and writing them to the card
        key_RW = pn532_mifareclassic_ReadDataBlock(&nfc, 4, card_key_1);
        if (key_RW)
        {
            // ESP_LOGI("PN532", "Reading block 5");
            // pn532_mifareclassic_AuthenticateBlock(&nfc, uid, uidLength, 5, 0, keyA);
            // key_RW = pn532_mifareclassic_ReadDataBlock(&nfc, 5, card_key_2);
        }
        else
        {
            ESP_LOGE("PN532", "Reading block failed");
        }
        // ESP_LOGI("PN532", "Block 4 Data: ");
        // for (int i = 0; i < 16; i++) {
        //     printf("%02X ", card_key_1[i]);
        // }
        // printf("\n");

        // ESP_LOGI("PN532", "Block 5 Data: ");
        // for (int i = 0; i < 16; i++) {
        //     printf("%02X ", card_key_2[i]);
        // }
        // printf("\n");
        memcpy(card_key,card_key_1,15);
        // memcpy(card_key+15,card_key_2,15);
        // ESP_LOGI("PN532", "Combime key: ");
        // for (int i = 0; i < 30; i++) 
        // {
        //     printf("%02X ", card_key[i]);
        // }
        int pass = 0;
        if (key_RW&&success)
        {
            pass = https_request(uid, card_key);   
        }
        free(card_key);
        card_key = NULL;
        if (pass)
        {
            xTaskCreate(&blink_task_green, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
        }
        else
        {
            xTaskCreate(&blink_task_red, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
        }
    }
}

// static rc522_handle_t scanner;

// static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
// {
//     rc522_event_data_t* data = (rc522_event_data_t*) event_data;

//     switch(event_id) {
//         case RC522_EVENT_TAG_SCANNED: {
//                 rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
//                 ESP_LOGI("RC522", "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
//             }
//             break;
//     }
// }

void app_main(void) {
    // queue = xQueueCreate(10, 128);
    // esp_log_level_set("*", ESP_LOG_INFO);
    // // to run this flash_writer(), please go to your sdk config and
    // // flash_writer(); //set Main task stack size : 8192
    // // otherwise you will get stack overflow D:
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_connect();
    obtain_time();
    // udp_server_task keep stack overflowing and I have no idea what to do with <- fixed
    // xTaskCreate(udp_server_task, "udp_server", 8192, (void*)AF_INET, 5, NULL);
    // udp_broadcaster();

    // testing area

    /*
    char testing_string[4096] = "abcdefg";
    size_t test_length = strlen(testing_string);
    ESP_LOGI("TEST","testing string length: %zu", test_length);
    uint8_t* tester = malloc(4096);
    memcpy(tester,testing_string,4096);
    test_length = string_with_AES(tester,test_length,false);
    ESP_LOGI("TEST","testing string length after encrypted:%zu",test_length);
    test_length = string_with_AES(tester,test_length,true);
    ESP_LOGI("TEST","testing string length after decrypted:%zu",test_length);
    ESP_LOGI("TEST","%s",tester);
    free(tester);
    */
    // flash_writer(); // change the esp_partition_find_first from "RSA_key" to "RSA_crt"
    // uint8_t *decrypted_rsa_private_key = malloc(4096); // Ensure your buffer is appropriately sized
    // flash_reader(decrypted_rsa_private_key, 4096,0);
    // uint8_t *decrypted_rsa_crt = malloc(4096);
    // flash_reader(decrypted_rsa_crt, 4096,1);

    //
    //     rc522_config_t config = {
    //     .spi.host = VSPI_HOST,
    //     .spi.miso_gpio = 19,
    //     .spi.mosi_gpio = 23,
    //     .spi.sck_gpio = 18,
    //     .spi.sda_gpio = 21, // no need
    //     // ss_gpio = 5
    // };
    // ESP_LOGE("TEST","change");
    // rc522_create(&config, &scanner);
    // rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    // rc522_start(scanner);

    xTaskCreate(&nfc_task, "nfc_task", 16384, NULL, 4, NULL);

    return;
}