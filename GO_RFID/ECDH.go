package main

import (
	"crypto/ecdh"
	"crypto/rand"
	"encoding/base64"
	"encoding/hex"
	"fmt"
)

var privateKeyHex, publicKeyBase64 string
	
func EDCH() {
	var Reader = rand.Reader
	// use MBEDTLS_ECP_DP_SECP256R1 curve
	fmt.Println("generating Key")
	ecdh_privateKey, err := ecdh.P256().GenerateKey(Reader)
	if err != nil {
		// TODO: handle error
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