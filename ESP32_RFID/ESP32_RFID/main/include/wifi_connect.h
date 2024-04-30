#ifndef WIFI_CONNECT
#define WIFI_CONNECT
#include "esp_event_base.h"

void wifi_connect();
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
#endif