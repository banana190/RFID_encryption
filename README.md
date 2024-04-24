# RFID_encryption
O: done , X: not started.

O Generate a EDCH key pair   
O Find esp32 on LAN using UDP   
O Burn HMAC key on Efuse  
O HMAC Derive AES for RSA private key and write it in flash   
WIP: RSA certificate <----   
? Use another key to encrypt the flash // Do we really need this?    
X AES Key derivation based on EDCH shared key    
X RC522  
X Encrypt UID by Key which derivation based on UID and EDCH shared key  
X Write Encrypt UID to card  
X Card attach fail handling  

Other part of this project: TOTP and Password  

The reason why the progress is slow because I've use two days try to make my esp prog working.    
However, All I get is :    
Error: JTAG scan chain interrogation failed: all ones    
like I cannot remember how many times I've connect and disconnect GPIO 12-15    
Well I guess it's time for debugging only using ESP_LOGI();    
Some things don't matter until they're lost, like debugger. Sadge    
    
