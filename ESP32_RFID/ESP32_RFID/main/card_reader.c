#include <esp_log.h>
#include "esp_system.h"
#include <inttypes.h>
#include <string.h>
#include "stdio.h"
#include "PN532.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RED_GPIO 2
#define GREEN_GPIO 15
#define BLUE_GPIO 4

extern pn532_t nfc;
extern uint8_t keyA[];

int card_write_on_block(uint8_t *uid, uint8_t *data)
{
    if (data == NULL) {
        ESP_LOGE("PN532", "Error: data pointer is NULL");
    }

    ESP_LOGI("PN532","card writing ");
    // for (int i = 0; i<30; i++) 
    // {
    //     printf("data[%d]: %c\n", i, (char)data[i]);
    // }
    uint8_t block4[16] ={0};
    // uint8_t block5[16] ={0};
    uint8_t ret;
    memcpy(block4+1, data, 15);
    ESP_LOGI("PN532","block4 ready");
    // memcpy(block5+1, data+15, 15);
    // ESP_LOGI("PN532","block5 ready");
    pn532_mifareclassic_AuthenticateBlock(&nfc, uid, 4, 4, 0, keyA);
    ret = pn532_mifareclassic_WriteDataBlock(&nfc, 4, block4);
    if (ret == 0)
    {
        ESP_LOGE("PN532","Error writing block 4");
    }
    // pn532_mifareclassic_AuthenticateBlock(&nfc, uid, 4, 5, 0, keyA);
    // ret = pn532_mifareclassic_WriteDataBlock(&nfc, 5, block5);
    // if (ret == 0)
    // {
    //     ESP_LOGE("PN532","Error writing block 5");
    // }
    return 0;
}

void blink_task_green(void *pvParameter)
{
    esp_rom_gpio_pad_select_gpio(GREEN_GPIO);
    gpio_set_direction(GREEN_GPIO, GPIO_MODE_OUTPUT);
    for (int i = 0; i <5;i++) {
        gpio_set_level(GREEN_GPIO, 0);
        vTaskDelay(900/portTICK_PERIOD_MS);
        gpio_set_level(GREEN_GPIO, 1);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    gpio_set_level(GREEN_GPIO, 0);
    vTaskDelete(NULL);
}


void blink_task_red(void *pvParameter)
{
    esp_rom_gpio_pad_select_gpio(RED_GPIO);
    gpio_set_direction(RED_GPIO, GPIO_MODE_OUTPUT);
    for (int i = 0; i <5;i++) {
        gpio_set_level(RED_GPIO, 0);
        vTaskDelay(900/portTICK_PERIOD_MS);
        gpio_set_level(RED_GPIO, 1);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    gpio_set_level(RED_GPIO, 0);
    vTaskDelete(NULL);
}

void blink_task_blue(void *pvParameter)
{
    esp_rom_gpio_pad_select_gpio(BLUE_GPIO);
    gpio_set_direction(BLUE_GPIO, GPIO_MODE_OUTPUT);
    for (int i = 0; i <3;i++) {
        gpio_set_level(BLUE_GPIO, 0);
        vTaskDelay(250/portTICK_PERIOD_MS);
        gpio_set_level(BLUE_GPIO, 1);
        vTaskDelay(250/portTICK_PERIOD_MS);
    }
    gpio_set_level(BLUE_GPIO, 0);
    vTaskDelete(NULL);
}


