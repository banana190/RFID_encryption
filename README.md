# New method a.k.a. I don't have time.     

Step One: ESP32 establish TLS connection to server with and send card data.    

Step Two: if valid server will send a new key to ESP32. Optional: also send a login URL.    

Step Three: password and TOTP login.       

Step Four: Server send OK to ESP32.     


# Working progress - ESP32       
O: done , X: not started.      
O Burn HMAC key on Efuse       
O HMAC Derive AES for RSA private key and crt. Write them into flash. 
O card read and write based on https://github.com/thanhbinh89/pn532-esp-idf     
O Generate new card key and write on card          
O Establish TLS connection      
O JSON send and response      
O Testing     

# Working progress - GO server    
O: done , X: not started.      
O OpenSSL generate RSA private key and self-signed certificate     
O Https handler     
O Json handler     
O Database based on sqlite3      
X Encrypt database (not hard I'll do if I have time)      
WIP: local IP finder <-----        
// copy on stack overflow and doesn't work well so I just input my current IP, will be fixed.       
O QRcode generate for user to scan based on https://github.com/mdp/qrterminal     
O HTML for login page (no css)  // idk why but clinet cannot read the .js under same root so I just put them in .html       
O Testing     




================ OLD progress ================
# How this work(old cool method)?    

Step One: ESP establish TLS connection to server with a ECDH public key E_E.~~    

Step Two: Server send a public key E_S.~~    


~~Step One: The server greets the ESP.~~    
~~Step Two: The ESP sends a signature to the server.~~   
~~Step Three: After confirming the signature, the server sends an ECDH public key to the ESP.~~
~~Step Four: Upon receiving the public key, the ESP checks its integrity and, if intact, sends its public key to the server.~~

Step Three: Using the shared key derived, the ESP read the encrypted UID and the one-time key on card and sends it to the server for confirmation.    
    
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
[Abandond]TCP connection handle   
WIP: Calculate shared secret key    
WIP: Derive AES key based on shared key    
WIP: JSON handle and respond
WIP: encrypt the database      
X Test everything

BTW I'll still finish this work with only TCP in the future because it's kinda fun, but currently the project deadline is close.        
    
