# RFID_encryption
O: done , X: not started.

O Generate a EDCH key pair   
O Find esp32 on LAN using UDP   
O Burn HMAC key on Efuse    
WIP: RSA certificate <----   
X HMAC Derive AES for RSA private key and write it in flash   
? Use another key to encrypt the flash // Do we really need this?
X AES Key derivation based on EDCH shared key
X RC522  
X Encrypt UID by Key which derivation based on UID and EDCH shared key  
X Write Encrypt UID to card  
X Card attach fail handling  

Other part of this project: TOTP and Password  
 
