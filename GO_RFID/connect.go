package main

import (
	"fmt"
	"net"
	"time"
)

const ESP_macStr = "e4-65-b8-83-d3-94"
const ESP_port = ":9527"
const serverPort = ":9527"

func main() {
    
    // UDP server
    udpAddr, err := net.ResolveUDPAddr("udp", serverPort)
	if err != nil {
		fmt.Println("Error resolving UDP address:", err)
		return
	}
	conn, err := net.ListenUDP("udp", udpAddr)
	if err != nil {
		fmt.Println("Error listening:", err)
		return
	}
	defer conn.Close()
	fmt.Println("Listening for UDP messages on port", serverPort)
    
	// UPD client
	ESP_ip := arp_finder(ESP_macStr)
	ESP_udpAddr, err := net.ResolveUDPAddr("udp", ESP_ip+ESP_port)
	if err != nil {
		fmt.Println("Error resolving UDP address:", err)
		return
	}
	sendConn, err := net.DialUDP("udp", nil, ESP_udpAddr)
	if err != nil {
		fmt.Println("Error connecting:", err)
		return
	}
	defer sendConn.Close()

	// key generate
    EDCH()
    fmt.Println("pirvate_key:", privateKeyHex)
    fmt.Println("public_key:", publicKeyBase64)

    // sending and listening
	const message = "Hello from yiyi's computer"
	buffer := make([]byte, 1024)
	for {
        // sending
		_, err := sendConn.Write([]byte(message))
		if err != nil {
			fmt.Println("Error sending message:", err)
			return
		}
		fmt.Println("Message sent successfully.")
        
        // recieving
		n, addr, err := conn.ReadFromUDP(buffer)
		if err != nil {
			fmt.Println("Error reading from UDP:", err)
			continue
		}
		fmt.Printf("Received message from %s: %s\n", addr, string(buffer[:n]))

		time.Sleep(1 * time.Second) 
	}
}

