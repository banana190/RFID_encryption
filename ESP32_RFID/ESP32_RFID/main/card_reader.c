#include <esp_log.h>
#include <inttypes.h>
#include <string.h>
#include "stdio.h"
#include "PN532.h"

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
    uint8_t block5[16] ={0};
    uint8_t ret;
    memcpy(block4+1, data, 15);
    ESP_LOGI("PN532","block4 ready");
    memcpy(block5+1, data+15, 15);
    ESP_LOGI("PN532","block5 ready");
    pn532_mifareclassic_AuthenticateBlock(&nfc, uid, 4, 4, 0, keyA);
    ret = pn532_mifareclassic_WriteDataBlock(&nfc, 4, block4);
    if (ret == 0)
    {
        ESP_LOGE("PN532","Error writing block 4");
    }
    pn532_mifareclassic_AuthenticateBlock(&nfc, uid, 4, 5, 0, keyA);
    ret = pn532_mifareclassic_WriteDataBlock(&nfc, 5, block5);
    if (ret == 0)
    {
        ESP_LOGE("PN532","Error writing block 5");
    }
    return 0;
}



