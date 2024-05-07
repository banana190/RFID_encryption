// this code is not belonging to yiyi(banana190)
// author : Adam

package main

import (
	"bufio"
	"crypto/hmac"
	"crypto/rand"
	"crypto/sha256"
	"encoding/base32"
	"encoding/binary"
	"fmt"
	"math"
	"os"
	"strings"
	"time"

	"github.com/skip2/go-qrcode"
)

func TOTP() {
secret, err := generateUniqueSecretKey()
if err != nil {
fmt.Println("Error generating secret key:", err)
return
}

// Display the base32 encoded secret key for debugging purposes
fmt.Printf("Secret Key (for debugging): %s\n", secret)

totpCode := generateTOTPWithSHA256(secret, 30)
fmt.Printf("Generated TOTP Code (for debugging): %s\n", totpCode)
// yiyi: edit here
/* original code
url := fmt.Sprintf("otpauth://totp/ExampleService:user@example.com?secret=%s&issuer=ExampleService&algorithm=SHA256", secret)
filePath, err := generateUniqueQRCode(url)
if err != nil {
fmt.Println("Failed to generate QR code:", err)
return
}
fmt.Println("QR Code saved to:", filePath)
*/
err = generateUniqueQRCode(secret)
if err != nil {
	fmt.Println("Failed to generate QR code:", err)
	return
}
// yiyi: edit end
attempts := 0
const maxAttempts = 3

scanner := bufio.NewScanner(os.Stdin)
for attempts < maxAttempts {
fmt.Print("Enter the TOTP code from your authenticator app: ")
scanner.Scan()
inputCode := strings.TrimSpace(scanner.Text())

if totpCode == inputCode {
fmt.Println("TOTP verification successful!")
return
} else {
fmt.Println("Invalid TOTP code entered.")
attempts++
}
}

fmt.Println("You have reached the maximum number of attempts.")
}

func generateUniqueSecretKey() (string, error) {
bytes := make([]byte, 20) // 160 bits
if _, err := rand.Read(bytes); err != nil {
return "", err
}
return base32.StdEncoding.WithPadding(base32.NoPadding).EncodeToString(bytes), nil
}

func generateTOTPWithSHA256(secret string, interval int64) string {
key, _ := base32.StdEncoding.WithPadding(base32.NoPadding).DecodeString(secret)
counter := time.Now().Unix() / interval
counterBytes := make([]byte, 8)
binary.BigEndian.PutUint64(counterBytes, uint64(counter))

hmac := hmac.New(sha256.New, key)
hmac.Write(counterBytes)
hash := hmac.Sum(nil)

offset := hash[len(hash)-1] & 0xf
binaryCode := int(hash[offset]&0x7f)<<24 | int(hash[offset+1])<<16 | int(hash[offset+2])<<8 | int(hash[offset+3])
otp := binaryCode % int(math.Pow10(6))
return fmt.Sprintf("%06d", otp)
}
// yiyi: edit here
/* original code
func generateUniqueQRCode(url string) (string, error) {
qrCodeData, err := qrcode.Encode(url, qrcode.Medium, 256)
if err != nil {
return "", err
}
filePath := "totp_qr.png"
if err := os.WriteFile(filePath, qrCodeData, 0644); err != nil {
return "", err
}
return filePath, nil
}
*/
func generateUniqueQRCode(totp_secret_key string) (error) {
	url := fmt.Sprintf("otpauth://totp/ExampleService:user@example.com?secret=%s&issuer=ExampleService&algorithm=SHA256", totp_secret_key)
	qrCodeData, err := qrcode.Encode(url, qrcode.Medium, 256)
	if err != nil {
	return  err
	}
	filePath := "totp_qr.png"
	if err := os.WriteFile(filePath, qrCodeData, 0644); err != nil {
	return  err
	}
	fmt.Println("QR Code saved to:", filePath)
	return nil
}
// yiyi: edit end