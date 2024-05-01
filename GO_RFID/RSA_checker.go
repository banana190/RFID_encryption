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
func parsePrivateKeyFromPEM(private_keyPEM []byte) (*rsa.PrivateKey, error) {
	block, _ := pem.Decode(private_keyPEM)
	if block == nil {
		return nil, errors.New("failed to decode PEM block containing private key")
	}
	private_key, err := x509.ParsePKCS1PrivateKey(block.Bytes)
	if err != nil {
		return nil, err
	}
	return private_key, nil
}

func parsePublicKeyFromPEM(public_keyPEM []byte) (*rsa.PublicKey, error) {
	block, _ := pem.Decode(public_keyPEM)
	if block == nil {
		return nil, errors.New("failed to decode PEM block containing public key")
	}
	public_key, err := x509.ParsePKCS1PublicKey(block.Bytes)
	if err != nil {
		return nil, err
	}
	return public_key, nil
}

func signature_checker(signature []byte, plaintext []byte) (int ,error){
	public_keyPEM, err := os.ReadFile("public_key.pem")
	if err != nil {
		fmt.Println("Failed to read private key file:", err)
		return 0,err
	}
	public_key, err := parsePublicKeyFromPEM(public_keyPEM)
	if err != nil {
		fmt.Println("Failed to parse private key:", err)
		return 0,err
	}
	err = rsa.VerifyPKCS1v15(public_key, crypto.SHA256, plaintext, signature)
    if err != nil {
        fmt.Println("Signature verification failed:", err)
        return 0,err
    }
    fmt.Printf("Signature verification successful\n")
	return 1,nil
}