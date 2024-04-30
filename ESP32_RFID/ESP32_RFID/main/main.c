#include <stdio.h>
#include <inttypes.h>
#include "string.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
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
#define BLINK_GPIO 2


#define BUF_SIZE (1024)
#define PORT 9527


char hostname[20];
int UDP_maximum_listener = 0;
static pn532_t nfc;



void blink_task(void *pvParameter)
{
    esp_rom_gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while (1)
    {
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(900 / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
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
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
        uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uidLength will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        success = pn532_readPassiveTargetID(&nfc, PN532_MIFARE_ISO14443A, uid, &uidLength, 0);

        if (success)
        {
            // Display some basic information about the card
            ESP_LOGI("PN532", "Found an ISO14443A card");
            ESP_LOGI("PN532", "UID Length: %d bytes", uidLength);
            ESP_LOGI("PN532", "UID Value:");
            esp_log_buffer_hexdump_internal("PN532", uid, uidLength, ESP_LOG_INFO);   
            vTaskDelay(1000 / portTICK_PERIOD_MS);         
        }
        else
        {
            // PN532 probably timed out waiting for a card
            ESP_LOGI("PN532", "Timed out waiting for a card");
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
    esp_log_level_set("*", ESP_LOG_INFO);
    // to run this flash_writer(), please go to your sdk config and
    // flash_writer(); //set Main task stack size : 8192
    // otherwise you will get stack overflow D:
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_connect();
    // udp_server_task keep stack overflowing and I have no idea what to do with <- fixed
    xTaskCreate(udp_server_task, "udp_server", 8192, (void*)AF_INET, 5, NULL);
    udp_broadcaster();

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
    // flash_writer();
    // uint8_t *decrypted_rsa_private_key = malloc(4096); // Ensure your buffer is appropriately sized
    // flash_reader(decrypted_rsa_private_key, 4096);


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
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
    xTaskCreate(&nfc_task, "nfc_task", 4096, NULL, 4, NULL);

    return;
}