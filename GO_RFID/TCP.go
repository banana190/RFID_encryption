package main

import (
	"bytes"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"log"
	"net"
)

func splitData(data []byte) [][]byte {
    return bytes.Split(data, []byte("\n"))
}

// header : U = UID + \n + AES_onetime_key + \n +RSA signature,
//              the RSA signature is based on the UID + AES_onetime_key
//        : E = ECDH public key
//	      : D = DONE
func handleConnection(conn net.Conn) {
	// largest packet might be UID + \n + AES256 + \n + RSA2048 sign which is 4 + 32 + 256 + 1 bytes
    buf := make([]byte, 512)
    n, err := conn.Read(buf)
    if err != nil {
        log.Println("Failed to read data from ESP32:", err)
        return
    }
	hexData := hex.EncodeToString(buf[:n])
    log.Println("Received data from ESP32:", hexData)
	TCP_data := splitData(buf)
	// this will be
	switch TCP_data[0][0] {
	case 'U':
		UID :=TCP_data[1]
		AES_OTK := TCP_data[2]
		SIGN := TCP_data[3]
		plaintext := append(UID, AES_OTK...)
		valid , err :=signature_checker(SIGN, plaintext)
		if err != nil {
			fmt.Println("signature validation failed:", err)
		}
		if valid != 1{
			fmt.Printf("Invalid signature\n")
		}
		EDCH() 
		publicKeyBytes, err := base64.StdEncoding.DecodeString(publicKeyBase64)
		_, err = conn.Write(publicKeyBytes)
		if err != nil {
			log.Println("Failed to write data back to client:", err)
			return
		}
		fmt.Printf("ECDH Public key sent successfully\n")

		// TODO : next step is wait the ECDH public key come and derive
		// I'll derive the key tmrw.
	case 'E':
		return
	default:
		return
	}

	defer conn.Close()
	
}

func start_TCP_server() {
    conn, err := net.Listen("tcp", ":9527")
    if err != nil {
        log.Fatal("Failed to start server:", err)
    }
	defer conn.Close()

    log.Println("Server started, waiting for ESP32 connections...")
    for {
        connection, err := conn.Accept()
        if err != nil {
            log.Println("Failed to accept connection:", err)
            continue
        }
        go handleConnection(connection)
    }
}

