package main

import (
	"crypto/ecdh"
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"io"

	"golang.org/x/crypto/hkdf"
)

var privateKeyHex, publicKeyBase64 string
var server_ecdh_PrivateKey ecdh.PrivateKey

func Base64_decoder(base64_data string) ([]byte){
	bytes_data ,err := base64.StdEncoding.DecodeString(base64_data)
	if err != nil {
		fmt.Println("Base64 decoding failed")
        return bytes_data
    }
	return bytes_data
}

func Shared_key_calculate(base64_from_ESP string) ([]byte, error) {
	// pubKeyBytes, err := base64.StdEncoding.DecodeString(base64_from_ESP)
	pubKeyBytes := Base64_decoder(base64_from_ESP)
	curve := ecdh.P256()
	curve.NewPublicKey(pubKeyBytes)
	privateKey, err := curve.NewPrivateKey(server_ecdh_PrivateKey.Bytes()) // this input is binary
	receivedPublicKey, err := curve.NewPublicKey(pubKeyBytes)
	sharedSecret, err := privateKey.ECDH(receivedPublicKey)
	Shared_AES_key :=EDCH_derive_AES_key(sharedSecret)
	return Shared_AES_key,err
}

func EDCH_derive_AES_key(sharedSecret []byte) []byte{	// WIP
	info := []byte("yiyi")
	salt := []byte("pepper")
	hash := sha256.New
	hkdf := hkdf.New(hash, sharedSecret, salt, info)

	aesKey := make([]byte, 32)
	if _, err := io.ReadFull(hkdf, aesKey); err != nil {
		fmt.Printf("cannot derive AES key: %v", err)
		return aesKey
	}
	fmt.Printf("AES key derived: %x\n", aesKey)
	return aesKey
}
// TODO: need a extra function to calculate the shared secret key

// func EDCH_secret_key() string {
// 	server_ecdh_PrivateKey.ECDH()
// 	return string(server_ecdh_PrivateKey)
// }
func EDCH_init() {
	var Reader = rand.Reader
	// use MBEDTLS_ECP_DP_SECP256R1 curve
	fmt.Println("generating Key")
	ecdh_privateKey, err := ecdh.P256().GenerateKey(Reader)
	if err != nil {
		fmt.Println("generate failed: ", err)
	}
	privateKeyHex = hex.EncodeToString(ecdh_privateKey.Bytes())
	ecdh_publicKey := ecdh_privateKey.PublicKey()   
	if err != nil {
		fmt.Println("public key generation failed: ", err)
	}

	publicKeyBase64 = base64.StdEncoding.EncodeToString(ecdh_publicKey.Bytes())

	// ecdsa_privateKey, err := ecdsa.GenerateKey(curve, rand.Reader)
	// if err != nil {
	// 	// error handling
	// }

	// publicKey := ecdsa_privateKey.PublicKey

	// // generate hex key pair
	// privateKeyBytes := ecdsa_privateKey.D.Bytes()
	// privateKeyHex = hex.EncodeToString(privateKeyBytes)

	
	// publicKeyBytes := elliptic.Marshal(curve, publicKey.X, publicKey.Y)
	// publicKeyBase64 = base64.StdEncoding.EncodeToString(publicKeyBytes)


}