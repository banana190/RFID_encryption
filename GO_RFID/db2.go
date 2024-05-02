package main

import (
	"bytes"
	"database/sql"
	"fmt"
	"log"

	_ "github.com/mattn/go-sqlite3"
)




func database_builder(){
	db, err := sql.Open("sqlite3", "./testing.db")
    if err != nil {
        log.Fatal(err)
    }
    defer db.Close()
// Thing need to store: UID(need encrypt), key-last-time-used , hashed password , TOTP key(need encrypt)
    _, err = db.Exec(`
        CREATE TABLE IF NOT EXISTS cards (
    		uid BLOB,
			hashed_password BLOB,
			key_this_time BLOB,
    		key_last_time BLOB,
    		totp_secret_key BLOB
		)
    `)
    if err != nil {
        log.Fatal(err)
    }
}
func database_register(uid []byte,hashed_password []byte, key_this_time []byte, totp_secret_key []byte){
	db, err := sql.Open("sqlite3", "./testing.db")
	if err != nil {
        log.Fatal(err)
    }
	defer db.Close()

    _, err = db.Exec("INSERT INTO cards(uid) VALUES(?)", uid)
    if err != nil {
        log.Fatal(err)
    }
    _, err = db.Exec("INSERT INTO cards(hashed_password) VALUES(?)", hashed_password)
    if err != nil {
        log.Fatal(err)
    }
	_, err = db.Exec("INSERT INTO cards(key_this_time) VALUES(?)", key_this_time)
    if err != nil {
        log.Fatal(err)
    }
	_, err = db.Exec("INSERT INTO cards(totp_secret_key) VALUES(?)", totp_secret_key)
    if err != nil {
        log.Fatal(err)
    }
    rows, err := db.Query("SELECT * FROM cards")
    if err != nil {
        log.Fatal(err)
    }
    defer rows.Close()

    fmt.Printf("CARDS:\n")
    for rows.Next() {
        var uid []byte
		var hashed_password []byte
        var key_last_time []byte
		var totp_secret_key []byte
		err := rows.Scan(&uid, &hashed_password, &key_last_time,&totp_secret_key)
        if err != nil {
            log.Fatal(err)
        }
        fmt.Printf("encrypt uid: %x\n", uid)
		fmt.Printf("hashed password: %x\n", hashed_password)
		fmt.Printf("key last time: %x\n", key_last_time)
		fmt.Printf("totp secret key: %x\n", totp_secret_key)
    }
    if err := rows.Err(); err != nil {
        log.Fatal(err)
    }
}
// return 1 if data matches, 0 otherwise
func database_checker(uid []byte, input []byte, arg string) (int, error) {
	db, err := sql.Open("sqlite3", "./testing.db")
	if err != nil {
        log.Fatal(err)
    }
	defer db.Close()


	switch arg{
	case "key_this_time":
		var key_this_time []byte
		row := db.QueryRow("SELECT key_this_time FROM cards WHERE uid = ?", uid)
		if err != nil {
			log.Fatal(err)
		}
		err = row.Scan(&key_this_time)
		if err != nil {
			log.Fatal(err)
		}
		if bytes.Equal(input, key_this_time){
			fmt.Printf("key matches")
			return 1,nil
		}
		return 0,nil
	case "key_last_time":
		var key_last_time []byte
		row := db.QueryRow("SELECT key_last_time FROM cards WHERE uid = ?", uid)
		if err != nil {
			log.Fatal(err)
		}
		err = row.Scan(&key_last_time)
		if err != nil {
			log.Fatal(err)
		}
		if bytes.Equal(input, key_last_time){
			fmt.Printf("key matches")
			return 1,nil
		}
		return 0,nil
	case "password":
		var password []byte
		row := db.QueryRow("SELECT hashed_password FROM cards WHERE uid = ?", uid)
		if err != nil {
			log.Fatal(err)
		}
		err = row.Scan(&password)
		if err != nil {
			log.Fatal(err)
		}
		if bytes.Equal(input, password){
			fmt.Printf("password matches")
			return 1,nil
		}
	case "TOTP":
		// this need to be fixed since idk what to store in database
		var TOTP []byte
		row := db.QueryRow("SELECT totp_secret_key FROM cards WHERE uid = ?", uid)
		if err != nil {
			log.Fatal(err)
		}
		err = row.Scan(&TOTP)
		if err != nil {
			log.Fatal(err)
		}
		if bytes.Equal(input, TOTP){
			fmt.Printf("password matches")
			return 1,nil
		}		
	default:
		fmt.Printf("wrong argument")
		return 0,nil
	}
	return 0,nil
}

func database_updater(uid []byte, input []byte, arg string) (int, error) {
	//WIP
	fmt.Printf("Work in progress")

	return 0,nil
}