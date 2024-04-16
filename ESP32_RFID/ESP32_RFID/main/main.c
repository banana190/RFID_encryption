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

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_connect();

    // udp_server_task keep stack overflowing and I have no idea what to do with
    xTaskCreate(udp_server_task, "udp_server", 8192, (void*)AF_INET, 5, NULL);
    udp_broadcaster();
    ecdh_key_exchange();
}