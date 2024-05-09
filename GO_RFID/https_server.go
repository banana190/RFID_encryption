package main

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"

	_ "github.com/mattn/go-sqlite3"
	"github.com/mdp/qrterminal/v3"
)

const localIP = "192.168.153.15"
var attempt_time = 0

type Request struct {
    Type string `json:"type"`
	UID string `json:"UID"`
	OtAK string `json:"One_time_Card_KEY"`
	New_OtAK string `json:"New_One_time_Card_KEY"`
	Password string `json:"PW"`
	TOTP string `json:"TOTP"`
}
// abandoned
// type ECDH struct {
// 	Type string `json:"ECDH"`
// 	PublicKey string `json:"PUBKEY"`
// }

// type Password_TOTP struct {
//     Type string `json:"type"`
// 	Password string `json:"PW"`
// 	UID string `json:"UID"`   //--> TODO: make the client to do URL query 
// 	TOTP string `json:"TOTP"`
// }

// type UID_KEY struct {
// 	Type string `json:"type"`
//     UID string `json:"UID"`
// 	OtAK string `json:"One_time_Card_KEY"`
// }

// type New_Card_Key struct {
// 	Type string `json:"type"`
// 	UID string `json:"UID"`
// 	New_OtAK string `json:"New_One_time_Card_KEY"`
// }

var checker bool
var passing bool
var test_once = true
const testing = true

func handleJSON(w http.ResponseWriter, r *http.Request, UID string) {
    var Req Request

    if err := json.NewDecoder(r.Body).Decode(&Req); err != nil {
        http.Error(w, "Invalid JSON", http.StatusBadRequest)
        return
    }
	defer r.Body.Close()

    switch Req.Type {
    case "Password":

        fmt.Println("Password:", Req.Password)
		passwordBytes := []byte(Req.Password)
		hash_fixed := sha256.Sum256(passwordBytes)
		hash := hash_fixed[:]
		bin_UID, err := hex.DecodeString(Req.UID)
		result ,err :=database_checker(bin_UID, hash, "password")
		if result != 1{
			fmt.Println("Invalid password")
			http.Error(w, "Invalid password", http.StatusBadRequest)
			attempt_time++
			return
		}
		if err != nil {
			fmt.Println("database_checker failed")
			return
		}
		fmt.Println("TOTP:" ,Req.TOTP)
		TOTP_bytes := []byte(Req.TOTP)

		result ,err =database_checker(bin_UID, TOTP_bytes, "TOTP")
		if result != 1{
			fmt.Println("Invalid TOTP")
			http.Error(w, "Invalid TOTP", http.StatusBadRequest)
			attempt_time++
			return
		}
		passing = true


    case "UID_KEY":

        fmt.Println("UID:", Req.UID)
		fmt.Println("Key:", Req.OtAK)
		bin_UID, err := hex.DecodeString(Req.UID)
		bin_OtAK, err := hex.DecodeString(Req.OtAK)
		if testing && test_once{
		hashed_password := sha256.Sum256([]byte("@Testing_password"))
		totp_secret_key,err := generateUniqueSecretKey()
		if err != nil{}
		err = generateUniqueQRCode(totp_secret_key)
		database_register(bin_UID, hashed_password[:], bin_OtAK, totp_secret_key)
		test_once = false
		}
		result,err := database_checker(bin_UID, bin_OtAK,"key_this_time")
		if err != nil {
			fmt.Println("database_checker failed")
			return
		}
		if result != 1{
			http.Error(w, "Invalid One time AES key", http.StatusBadRequest)
			w.Header().Set("Content-Type", "text/plain")
			w.WriteHeader(http.StatusOK)
			w.Write([]byte("IS_OK:NO"))
			return
		}
		w.Header().Set("Content-Type", "text/plain")
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("IS_OK:OK"))
		checker = true

	case "New_Card_Key":
		if checker {
			fmt.Println("UID:", Req.UID)
			fmt.Println("Key:", Req.New_OtAK)
			bin_UID, err := hex.DecodeString(Req.UID)
			bin_New_OtAK, err := hex.DecodeString(Req.New_OtAK)
			if err != nil {
				fmt.Println("err declared and not used SCHIZO")
			}
			err = database_updater(bin_UID,bin_New_OtAK,Req.Type)
			if err != nil {
				http.Error(w, "Write key wrong", http.StatusBadRequest)
				w.Header().Set("Content-Type", "text/plain")
				w.WriteHeader(http.StatusOK)
				w.Write([]byte("IS_OK:NO"))
			} else{
				w.Header().Set("Content-Type", "text/plain")
				w.WriteHeader(http.StatusOK)
				w.Write([]byte("IS_OK:OK"))
			}
			r.Body.Close()

			// generate a qrcode for user to scan
			// localIP, err := getLocalIP()
			// since the getLocalIP() not working very well so for demo I change the IP by my self
			// will be fixed in future
			content := fmt.Sprintf("https://%s?UID=%s", localIP, Req.UID)
			qrterminal.Generate(content, qrterminal.L, os.Stdout)
			
			
		}
    default:
        http.Error(w, "Unknown Type", http.StatusBadRequest)
        return
    }

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
		
		http.ServeFile(w, r, "input_totp.html") // not done yet

	case "/json": // this is for esp32
		if r.Method != http.MethodPost {
            http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
            return
        }
        handleJSON(w, r, "")
	case "/OPEN":
		if passing && checker {
			fmt.Println("passing:", passing, "checker:", checker)
			w.Header().Set("Content-Type", "text/plain")
            w.WriteHeader(http.StatusOK)
			passing = false
			checker = false
			attempt_time = 0
		}else if attempt_time >= 2 {
			attempt_time = 0
			http.Error(w, "BadRequest", http.StatusBadRequest)
		}else{
            http.Error(w, "Unauthorized", http.StatusUnauthorized)
        }

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
