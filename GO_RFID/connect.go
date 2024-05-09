package main

import (
	"log"
	"os"

	"github.com/hashicorp/mdns"
)

func main() {
	database_builder()

	host, _ := os.Hostname()
	info := []string{"yiyi_testing"}
	service, err := mdns.NewMDNSService(host, "_http._tcp", "group11_3auth.", "", 8080, nil, info)
	if err != nil {
		log.Fatal(err)
	}

	// Create the mDNS server, defer shutdown
	server, err := mdns.NewServer(&mdns.Config{Zone: service})
	if err != nil {
		log.Fatal(err)
	}
	defer server.Shutdown()
	start_https_server()
	// uid := []byte{0x23, 0x08, 0x1B, 0x14, 0x00, 0x00, 0x00}
	// hashed_password := []byte{0x00}
	// Otak := []byte{0x00}
	// for i := 0; i < 29; i++ {
	// 	Otak = append(Otak, 0x00)
	// }
	// totp_secret_key := []byte{0x00}
	// database_register(uid, hashed_password, Otak, totp_secret_key)
	// EDCH_init()
	// fmt.Println("pirvate_key:", privateKeyHex)
	// fmt.Println("public_key:", publicKeyBase64)

	// start_TCP_server()
}

// this value of err is never used SCHIZO