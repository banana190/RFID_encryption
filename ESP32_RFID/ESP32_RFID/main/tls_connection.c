// ref: https://github.com/espressif/esp-idf/blob/v5.2.1/examples/protocols/https_request/main/https_request_example_main.c
// I'm using self signed certificate so the server_root_cert is same as local_server_cert.

#include "string.h"
#include "stdio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "driver/gpio.h"
#define BLUE_GPIO 4

#include "esp_tls.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "flash_write_rsa_key.h"
#include "card_reader.h"


#define WEB_SERVER "https://192.168.153.15"  // LAN
#define WEB_PORT "443" 
#define HOST "Host: 192.168.153.15\r\n"
#define WEB_URL "No domain and no fixed ip Erm"

#define Flash_Size 4096
esp_tls_cfg_t TLS_config;
static esp_tls_client_session_t *saved_client_session = NULL;

extern int64_t end_time;
extern int64_t start_time;

void generate_random_hex(uint8_t *buffer, size_t length)
{
    for (int i = 0; i < length; i++) {
        buffer[i] = esp_random() % 256;  
    }
}

void bytes_to_hex_string(const uint8_t *bytes, size_t length, char *hex_string) {
    for (size_t i = 0; i < length; ++i) {
        sprintf(hex_string + i * 2, "%02X", bytes[i]);
    }
}

int https_request(uint8_t *uid, uint8_t *card_key)
{
    uint8_t *key = malloc(Flash_Size);
    size_t key_size = flash_reader(key, Flash_Size,0);
    uint8_t *crt = malloc(Flash_Size);
    size_t cert_size = flash_reader(crt, Flash_Size,1);
    bool system_pass = false;



    if (key_size == 0) {
        ESP_LOGE("TLS", "Failed to read and decrypt private key");
        goto clear;
    }
    if (cert_size == 0) {
        ESP_LOGE("TLS", "Failed to read and decrypt private key");
        goto clear;
    }    

    esp_tls_cfg_t cfg = {
        .cacert_buf = crt, 
        .cacert_bytes = cert_size,
        .clientkey_buf = key,
        .clientkey_bytes = key_size,
        .clientcert_buf = crt,
        .clientcert_bytes = cert_size,
        .client_session = saved_client_session,
        .skip_common_name = true,
    };

    const char *server_url = WEB_SERVER;
    uint8_t buf[512];
    int ret, len;

    esp_tls_t *tls = esp_tls_conn_http_new(server_url, &cfg);
    if (!tls) {
        ESP_LOGE("TLS", "Failed to connect to server");
        goto clear;
    }

    ESP_LOGI("TLS", "Connected to server");

    if (saved_client_session == NULL) 
    {
        saved_client_session = esp_tls_get_client_session(tls);
    }

    char hex_uid[15]; // 7*2+1
    bytes_to_hex_string(uid, 7, hex_uid);
    // char hex_key[61]; // 30*2+1
    // bytes_to_hex_string(card_key, 30, hex_key);
    char hex_key[31]; // 15*2+1
    bytes_to_hex_string(card_key, 15, hex_key);

    ESP_LOGI("TLS","UID: %s ,Key: %s",hex_uid,hex_key);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "UID_KEY");
    cJSON_AddStringToObject(root, "UID", hex_uid); // using hex
    cJSON_AddStringToObject(root, "One_time_Card_KEY", hex_key);
    char *json_data = cJSON_PrintUnformatted(root);
    size_t json_data_len = strlen(json_data);

    char request[1024];
    snprintf(request, sizeof(request),
             "POST /json HTTP/1.1\r\n"
              HOST
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "User-Agent: esp-idf/1.0 esp32\r\n"
             "\r\n%s",
             json_data_len, json_data);

    cJSON_Delete(root);
    
    ret = esp_tls_conn_write(tls, request, strlen(request));
    if (ret < 0) {
        ESP_LOGE("TLS", "Failed to send request");
        esp_tls_conn_destroy(tls);
        goto clear;
    }
    free(json_data);

    do {
        len = sizeof(buf) - 1;
        memset(buf, 0, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);
        if (ret <= 0) 
            break;
        buf[ret] = '\0';
        ESP_LOGI("TLS", "Received response: %s", buf);
        if (strstr((const char *)buf, "200 OK")) 
            break;
    } while (1);
    if (strstr((const char *)buf, "200 OK"))
    {
        // ESP_LOGI("TLS", "First response completed");
        uint8_t random_bytes[31] = {0};
        // ESP_LOGI("TLS", "Generating random bytes");
        generate_random_hex(random_bytes,15);
        // ESP_LOGI("TLS", "Writing new key to card");
        int err = card_write_on_block(uid,random_bytes);
        // from here the card can leave from the reader.
        ESP_LOGI("RFID_READER", "Card can detach");
        if (err)
        {
            ESP_LOGI("PN532", "Failed to write the card");
        }
        else
        {
            end_time = esp_timer_get_time();
            int64_t elapsed_time = end_time - start_time;
            start_time = 0;
            end_time = 0;
            ESP_LOGI("TIMER","Elapsed time: %lld microseconds\n", elapsed_time);
            ESP_LOGI("TLS", "Writing new key to card success");
            char hex_random_bytes[31];
            for (int i = 0; i < 15; i++) {
            sprintf(hex_random_bytes + 2 * i, "%02X", random_bytes[i]);
            }

            cJSON *root2 = cJSON_CreateObject();
            cJSON_AddStringToObject(root2, "type", "New_Card_Key");
            cJSON_AddStringToObject(root2, "UID", hex_uid); // using hex
            cJSON_AddStringToObject(root2, "New_One_time_Card_KEY", hex_random_bytes);
            char *new_key_data = cJSON_PrintUnformatted(root2);
            size_t new_key_data_len = strlen(new_key_data);
            char new_key_request[512];
            snprintf(new_key_request, sizeof(new_key_request),
                        "POST /json HTTP/1.1\r\n"
                         HOST
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "User-Agent: esp-idf/1.0 esp32\r\n"
                        "\r\n%s",
                        new_key_data_len, new_key_data);

            ret = esp_tls_conn_write(tls, new_key_request, strlen(new_key_request));
            cJSON_Delete(root2);
            if (ret < 0) 
            {
                ESP_LOGE("TLS", "Failed to send success request");
                goto clear;
            }

            do { 
                len = sizeof(buf) - 1;
                memset(buf, 0, sizeof(buf));
                ret = esp_tls_conn_read(tls, (char *)buf, len);
                if (strstr((const char *)buf, "IS_OK:NO")) 
                {
                    ESP_LOGE("TLS", "Invalid card Key");
                    esp_tls_conn_destroy(tls);
                    goto clear;
                }
                if (ret <= 0) 
                    break;
                buf[ret] = '\0';
                ESP_LOGI("TLS", "Received response: %s", buf);
                break;
            } while (1);


            ESP_LOGI("TLS", "waiting user login");
            char heartbeat_request[64];

            snprintf(heartbeat_request,64,
                "HEAD /OPEN HTTP/1.1\r\n"
                 HOST
                "\r\n"
                );
            len = sizeof(buf) - 1;
            memset(buf, 0, sizeof(buf));
            do { 
                ret = esp_tls_conn_write(tls, heartbeat_request, strlen(heartbeat_request));
                ret = esp_tls_conn_read(tls, (char *)buf, len);
                if (ret <= 0) 
                    break;
                buf[ret] = '\0';
                ESP_LOGI("TLS", "Received response: %s", buf);
                if (strstr((const char *)buf, "200 OK")) 
                {
                    ESP_LOGI("TLS", "Received pass");
                    system_pass = true;
                    break;
                }
                if (strstr((const char *)buf, "400 Bad")) 
                {
                    ESP_LOGE("TLS", "Attempt time limit exceeded");
                    break;
                }
                xTaskCreate(&blink_task_blue, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
                vTaskDelay(pdMS_TO_TICKS(2500));
            } while (1);
        }
    }
    esp_tls_conn_destroy(tls);
clear:
    ESP_LOGI("TLS", "Connection closed");
    free(key);
    free(crt);
    if (system_pass)
        return 1;
    else
        return 0;
}