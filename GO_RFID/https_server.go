package main

import (
	"fmt"
	"log"
	"net/http"

	_ "github.com/mattn/go-sqlite3"
)

func handler(w http.ResponseWriter, r *http.Request) {
	path := r.URL.Path
    switch path {
    case "/":
		// load UID here
        http.ServeFile(w, r, "index.html")
    case "/Password":
		err := r.ParseForm()
    	if err != nil {
        http.Error(w, "Failed to parse form", http.StatusBadRequest)
        return
		}
		password := r.PostFormValue("text")
		w.Header().Set("Content-Type", "text/plain")
    	// fmt.Fprintf(w, "Received text: %s\n", password) // just for debugging
		fmt.Println("Received text:", password)
		
		http.ServeFile(w, r, "input_totp.html") // not done yet
	case "/TOTP":
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

    default:
        http.NotFound(w, r)
    }
}
// NO domain EWWW
func start_https_server() {
    http.HandleFunc("/", handler)
    certFile := "certificate.crt" 
    keyFile := "private_key.pem" 
    log.Fatal(http.ListenAndServeTLS(":443", certFile, keyFile, nil))
}
