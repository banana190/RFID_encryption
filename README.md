# How this work?    
Step One: The server greets the ESP.    
    
Step Two: The ESP sends a signature to the server.    
    
Step Three: After confirming the signature, the server sends an ECDH public key to the ESP.    
    
Step Four: Upon receiving the public key, the ESP checks its integrity and, if intact, sends its public key to the server.    
    
Step Five: Using the shared key derived, the ESP read the encrypted UID and the one-time key on card and sends it to the server for confirmation.    
    
Step Six: After receiving confirmation from the server, the ESP writes this AES key onto the card.   
    
Step Seven: The ESP sends a success message for the key writing, and the server updates the one-time key for that UID.    
    

# Working progress - ESP32           
O: done , X: not started.    

O Generate a EDCH key pair   
O Find esp32 on LAN using UDP   
O Burn HMAC key on Efuse  
O HMAC Derive AES for RSA private key and write it in flash   
O RSA certificate <---- done     
O AES Key derivation based on EDCH shared key     
WIP: ~~RC522~~ PN532        
X Write one time AES key to card    
X Card attach fail handling  
- Encrypt UID by Key which derivation based on UID and EDCH shared key ---> do on server side    
Other part of this project: TOTP and Password    

# Working progress - GO server    
O: done , X: not started.    

O HTTPS so password and TOTP will be TLS1.2     
O Database sqlite3    
WIP:  TCP connection handle    
WIP:  Calculate shared secret key    
X Derive AES key based on shared key    
X encrypt the database      
X Test everything

The reason why the progress is slow because I've use two days try to make my esp prog working.    
However, All I get is :    
Error: JTAG scan chain interrogation failed: all ones    
like I cannot remember how many times I've connect and disconnect GPIO 12-15    
Well I guess it's time for debugging only using ESP_LOGI();    
Some things don't matter until they're lost, like debugger. Sadge    
    
