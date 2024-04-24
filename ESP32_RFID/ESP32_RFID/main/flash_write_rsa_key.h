#ifndef FLASH_WRITE_RSA_KEY
#define FLASH_WRITE_RSA_KEY

void flash_writer();
void flash_reader(uint8_t *decrypted_rsa_private_key,size_t flash_size);
size_t string_with_AES(uint8_t *input_string,size_t length,bool mode);
#endif