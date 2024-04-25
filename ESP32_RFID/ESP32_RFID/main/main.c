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
// #include "mdns.h"
// my functions
#include "wifi_connect.h"
#include "ecdh.h" 
#include "publickey_reciever.h"
#include "flash_write_rsa_key.h"


//package for udp task
#include <sys/param.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>


#define BUF_SIZE (1024)
#define PORT 9527


char hostname[20];
int UDP_maximum_listener = 0;


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
    unnamed();
    wifi_connect();
    // udp_server_task keep stack overflowing and I have no idea what to do with <- fixed
    xTaskCreate(udp_server_task, "udp_server", 8192, (void*)AF_INET, 5, NULL);
    udp_broadcaster();
    // ecdh_key_exchange();

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
    return;
}