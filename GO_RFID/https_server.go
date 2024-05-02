package main

import (
	"crypto/sha256"
	"encoding/json"
	"fmt"
	"log"
	"net/http"

	_ "github.com/mattn/go-sqlite3"
)


type Request struct {
    Type string `json:"type"`
}

type ECDH struct {
	Type string `json:"ECDH"`
	PublicKey string `json:"PUBKEY"`
}

type Password struct {
    Type string `json:"Password"`
	Password string `json:"PW"`
}

type UID_KEY struct {
	Type string `json:"UID_KEY"`
    UID string `json:"UID"`
	OtAK string `json:"One_time_AES_KEY"`
}

type TOTP_PIN struct {
	Type string `json:"TOTP_PIN"`
	PIN string `json:"PIN"`
}

func handleJSON(w http.ResponseWriter, r *http.Request, UID string) {
    var commonReq Request

    if err := json.NewDecoder(r.Body).Decode(&commonReq); err != nil {
        http.Error(w, "Invalid JSON", http.StatusBadRequest)
        return
    }
	defer r.Body.Close()


    switch commonReq.Type {
    case "Password":
        var Password_request Password
        if err := json.NewDecoder(r.Body).Decode(&Password_request); err != nil {
            http.Error(w, "Invalid Password JSON", http.StatusBadRequest)
            return
        }
        fmt.Println("Password:", Password_request.Password)
		passwordBytes := []byte(Password_request.Password)
		hash_fixed := sha256.Sum256(passwordBytes)
		hash := hash_fixed[:]
		UID_bytes := Base64_decoder(UID)
		result ,err :=database_checker(UID_bytes, hash, "password")
		if result != 1{
			http.Error(w, "Invalid password", http.StatusBadRequest)
			return
		}
		if err != nil {
			fmt.Println("database_checker failed")
			return
		}
		// TODO: can we input TOTP and password at the same time so that I don't have to handle the next json
		http.Redirect(w, r, "/TOTP", http.StatusFound)


    case "UID_KEY":
        var uid_key UID_KEY
        if err := json.NewDecoder(r.Body).Decode(&uid_key); err != nil {
            http.Error(w, "Invalid UID_KEY JSON", http.StatusBadRequest)
            return
        }
        fmt.Println("UID:", uid_key.UID)
		fmt.Println("Key:", uid_key.OtAK)
		UID_bytes := Base64_decoder(uid_key.UID)
		OtAK_bytes := Base64_decoder(uid_key.OtAK)
		result,err := database_checker(UID_bytes, OtAK_bytes,"key_this_time")
		if result != 1{
			http.Error(w, "Invalid One time AES key", http.StatusBadRequest)
			return
		}
		if err != nil {
			fmt.Println("database_checker failed")
			return
		}
		// TODO: update OtAK here
		database_updater(UID_bytes,OtAK_bytes,"one_time_key")
		response := map[string]string{
			"UID" : uid_key.UID,
			// "Key": uid_key.OtAK,    // idk is it necessary to hide the UID since I have to write a extra quary
									   // like last_time_key -> password 
		}
		jsonResponse, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}

		redirectURL := "/password_input?data=" + string(jsonResponse)
		http.Redirect(w, r, redirectURL, http.StatusFound)

	case "TOTP_PIN":
		var totp_pin TOTP_PIN
		if err := json.NewDecoder(r.Body).Decode(&totp_pin); err != nil {
			http.Error(w, "Invalid TOTP_PIN JSON", http.StatusBadRequest)
            return
		}
		// if TOTP match
		// send json to ESP32

    default:
        http.Error(w, "Unknown Type", http.StatusBadRequest)
        return
    }
    w.Header().Set("Content-Type", "text/plain")
    fmt.Fprintln(w, "Request processed")
}

func path_handler(w http.ResponseWriter, r *http.Request) {
	path := r.URL.Path
    switch path {
    case "/":
		// load UID here
        http.ServeFile(w, r, "index.html")
    case "/password_input":
		if r.Method != http.MethodPost {
            http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
            return
        }
		UID := r.URL.Query().Get("UID")
		handleJSON(w, r,UID)

		// err := r.ParseForm()
    	// if err != nil {
        // http.Error(w, "Failed to parse form", http.StatusBadRequest)
        // return
		// }
		// password := r.PostFormValue("text")
		// w.Header().Set("Content-Type", "text/plain")
    	// // fmt.Fprintf(w, "Received text: %s\n", password) // just for debugging
		// fmt.Println("Received text:", password)
		
		http.ServeFile(w, r, "input_totp.html") // not done yet
	case "/TOTP":
		if r.Method != http.MethodPost {
            http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
            return
        }
		err := r.ParseForm()
    	if err != nil {
        http.Error(w, "Failed to parse form", http.StatusBadRequest)
        return
		}
		text := r.PostFormValue("text")
		w.Header().Set("Content-Type", "text/plain")
    	fmt.Fprintf(w, "Received text: %s\n", text)
		fmt.Println("Received text:", text)
		// TODO: function handle TOTP
		http.ServeFile(w, r, "Result.html")
	case "/json":
		if r.Method != http.MethodPost {
            http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
            return
        }
        handleJSON(w, r, "")
	// case "/ESP32_ECDH":
	// 	if r.Method != http.MethodPost {
    //         http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
    //         return
    //     }
	// 	// this might need to change since I haven't change the ESP from TCP to TLS
	// 	err := r.ParseForm()
    // 	if err != nil {
    //     http.Error(w, "Failed to parse form", http.StatusBadRequest)
    //     return
	// 	}
	// 	text := r.PostFormValue("text")
	// 	w.Header().Set("Content-Type", "text/plain")
    // 	fmt.Fprintf(w, "Received text: %s\n", text)
	// 	fmt.Println("Received text:", text)
	// 	Shared_key_calculate(text)
	// case "/ESP32_UID": // Since the connection is TLS so there is no more Signature check
	// 	if r.Method != http.MethodPost {
	// 		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
	// 		return
	// 	}	
	// 	err := r.ParseForm()
    // 	if err != nil {
    //     http.Error(w, "Failed to parse form", http.StatusBadRequest)
    //     return
	// 	}
	// 	text := r.PostFormValue("text") // WIP
	// 	w.Header().Set("Content-Type", "text/plain")
    // 	fmt.Fprintf(w, "Received text: %s\n", text)
	// 	fmt.Println("Received text:", text)
	// 	Shared_key_calculate(text)
    default:
        http.NotFound(w, r)
    }
}
func start_https_server() {
    http.HandleFunc("/", path_handler)
    certFile := "certificate.crt" 
    keyFile := "private_key.pem" 
    log.Fatal(http.ListenAndServeTLS(":443", certFile, keyFile, nil))
}
