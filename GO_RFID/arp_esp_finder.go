package main

import (
	"bufio"
	"fmt"
	"os/exec"
	"strings"
)

func arp_finder(ESP_macStr string) string {
    // This mac address is the esp32 I have used, change it to a list if have more than one device
    
    ESP_ip := ""
    // TODO: need to change in Ubuntu since this is based on Windows command
    cmd := exec.Command("arp", "-a")
    output, err := cmd.Output()
    if err != nil {
        fmt.Println("Arp error:", err)
        return "error"
    }

    scanner := bufio.NewScanner(strings.NewReader(string(output)))
    for scanner.Scan() {
        line := scanner.Text()
        fields := strings.Fields(line)
        if len(fields) >= 3 && fields[1] == ESP_macStr {
            fmt.Printf("Esp's IP address：%s\n", fields[0])
            ESP_ip = fields[0]
        }
    }
    if err := scanner.Err(); err != nil {
        fmt.Println("Arp table output scan error:", err)
    }	
	return ESP_ip
}