# burn key in efuse

Check your esp32 with :

espefuse.py --port <your_esp_device_COM> summary
e.g. espefuse.py --port COM5 summary

# As the [documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/efuse.html)

ESP32 has 4 eFuse blocks each of the size of 256 bits (not all bits are available):

EFUSE_BLK0 is used entirely for system purposes;

EFUSE_BLK1 is used for flash encrypt key. If not using that Flash Encryption feature, they can be used for another purpose;

EFUSE_BLK2 is used for security boot key. If not using that Secure Boot feature, they can be used for another purpose;

**EFUSE_BLK3 can be partially reserved for the custom MAC address, or used entirely for user application. Note that some bits are already used in ESP-IDF.**

So I use BLK3 to save the HMAC key, please check your ESP32's efuse haven't been occupied.

A new ESP32 should be look like this:

BLOCK1 (BLOCK1) Flash encryption key
= 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W

BLOCK2 (BLOCK2) Security boot key
= 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W

BLOCK3 (BLOCK3) Variable Block 3
= 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W

Once you make sure the block you want to use, prepare the HMAC key: hmac_key.bin which is 32 bytes.

# **WARNING: This is irreparable**
Burn your key with:
espefuse.py --port <your_esp_device_COM> burn_key <BLOCK1/2/3> hmac_key.bin

e.g.
espefuse.py --port COM5 burn_key BLOCK3 hmac_key.bin
