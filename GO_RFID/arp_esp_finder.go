package main

import (
	"bufio"
	"fmt"
	"net"
	"os/exec"
	"strings"
)

// from stack overflow. classic coding moment
func getLocalIP() (string, error) {
    interfaces, err := net.Interfaces()
    if err != nil {
        return "", err
    }

    for _, iface := range interfaces {
        if iface.Name != "wlan0" {
            continue
        }

        addrs, err := iface.Addrs()
        if err != nil {
            return "", err
        }

        for _, addr := range addrs {
            var ip net.IP

            switch v := addr.(type) {
            case *net.IPNet:
                ip = v.IP
            case *net.IPAddr:
                ip = v.IP
            }
            if ip != nil && ip.To4() != nil {
                return ip.String(), nil
            }
        }
    }

    return "", fmt.Errorf("No IPv4 address found on wlan0")
}

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