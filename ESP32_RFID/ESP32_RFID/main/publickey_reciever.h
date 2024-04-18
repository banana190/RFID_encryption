#ifndef PUBLICKEY_RECIEVER
#define PUBLICKEY_RECIEVER

void udp_server_task(void *pvParameters);
void udp_broadcaster();
char Sign_with_SHA256();
void flash_writer();
#endif