package main

import (
	"crypto"
	"crypto/rand"
	"crypto/rsa"
	"crypto/sha256"
	"crypto/x509"
	"encoding/hex"
	"encoding/pem"
	"errors"
	"fmt"
	"os"
)

func key_tester() {
	privateKeyPEM, err := os.ReadFile("private_key.pem")
	if err != nil {
		fmt.Println("Failed to read private key file:", err)
		return
	}

	privateKey, err := parsePrivateKeyFromPEM(privateKeyPEM)
	if err != nil {
		fmt.Println("Failed to parse private key:", err)
		return
	}

	message := []byte{0x41, 0x00} // A\0
	fmt.Println(string(message))
	hash := sha256.Sum256(message)

	signature, err := rsa.SignPKCS1v15(rand.Reader, privateKey, crypto.SHA256, hash[:])
	if err != nil {
		fmt.Println("Failed to sign message:", err)
		return
	}
	signatureHex := hex.EncodeToString(signature)
	fmt.Println("Signature (Hex):", signatureHex)
}
func parsePrivateKeyFromPEM(privateKeyPEM []byte) (*rsa.PrivateKey, error) {
	block, _ := pem.Decode(privateKeyPEM)
	if block == nil {
		return nil, errors.New("failed to decode PEM block containing private key")
	}

	privateKey, err := x509.ParsePKCS1PrivateKey(block.Bytes)
	if err != nil {
		return nil, err
	}

	return privateKey, nil
}
