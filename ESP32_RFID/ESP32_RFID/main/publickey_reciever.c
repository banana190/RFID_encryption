/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "mbedtls/sha256.h"

#define PORT 9527
#define CHAR_SET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!~@#$^()_+=-"
#define Signiture_length 16

static const char *TAGg = "example";

char* generate_random_string(int length) {
    char* random_string = malloc((length + 1) * sizeof(char));
    if (random_string == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < length; i++) {
        int random_index = esp_random() % (strlen(CHAR_SET));
        random_string[i] = CHAR_SET[random_index];
    }
    random_string[length] = '\0';
    
    return random_string;
}


void udp_server_task(void *pvParameters) {
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE("UDP", "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI("UDP", "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE("UDP", "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI("UDP", "Socket bound, port %d", PORT);

        while (1) {

            ESP_LOGI("UDP", "Waiting for data");
            struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE("UDP", "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's IP address as string
                if (source_addr.sin_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI("UDP", "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI("UDP", "%s", rx_buffer);

                const char *tx_buffer = "Hello from ESP32";
                source_addr.sin_port = htons(PORT);
                for (int i = 0; i < 2; i++) {
                int err = sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                ESP_LOGI("UDP", "Message sent to client: %s", tx_buffer);
                if (err < 0) {
                    ESP_LOGE("UDP", "Error occurred during sending: errno %d", errno);
                }
                
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE("UDP", "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void udp_broadcaster()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE("UDP", "Failed to create socket");
        vTaskDelay(portMAX_DELAY);
    }

    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ESP_LOGE("UPD", "Failed to set broadcast option");
        close(sock);
        vTaskDelay(portMAX_DELAY);
    }
    const char *message = "This is ESP32 broadcasting"; // TODO: This message should be encrypted
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr);
}

char Sign_with_SHA256()//WIP
{
    char* signature = generate_random_string(Signiture_length);
    return signature;
}