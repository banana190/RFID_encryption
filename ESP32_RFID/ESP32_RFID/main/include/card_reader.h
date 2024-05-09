#ifndef CARD_READER
#define CARD_READER
#include "PN532.h"
extern pn532_t nfc;

int card_write_on_block(uint8_t *uid, uint8_t *data);
void blink_task_red(void *pvParameter);
void blink_task_green(void *pvParameter);
void blink_task_blue(void *pvParameter);

#endif