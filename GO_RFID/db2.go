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
    		totp_secret_key STRING
		)
    `)
    if err != nil {
        log.Fatal(err)
    }
}
func database_register(uid []byte,hashed_password []byte, key_this_time []byte, totp_secret_key string){
	db, err := sql.Open("sqlite3", "./testing.db")
	if err != nil {
        log.Fatal(err)
    }
	defer db.Close()

	_, err = db.Exec("INSERT INTO cards(uid, hashed_password, key_this_time, key_last_time, totp_secret_key) VALUES(?, ?, ?, ?, ?)", uid, hashed_password, key_this_time, key_this_time, totp_secret_key)
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
		var key_this_time []byte
        var key_last_time []byte
		var totp_secret_key string
		err := rows.Scan(&uid, &hashed_password,&key_this_time, &key_last_time,&totp_secret_key)
        if err != nil {
            log.Fatal(err)
        }
        fmt.Printf("encrypt uid: %x\n", uid)
		fmt.Printf("hashed password: %x\n", hashed_password)
		fmt.Printf("key this time: %x\n", key_this_time)
		fmt.Printf("key last time: %x\n", key_last_time)
		fmt.Printf("totp secret key: %s\n", totp_secret_key)
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
	//TESTING
	row := db.QueryRow("SELECT * FROM cards WHERE uid = ?", uid)
	var uid_out, hashed_password, key_this_time, key_last_time, totp_secret_key []byte

	err = row.Scan(&uid_out, &hashed_password, &key_this_time, &key_last_time, &totp_secret_key)
	if err == sql.ErrNoRows {
	    fmt.Println("No rows found.")
	    return -1, nil 
	} else if err != nil {
	    log.Fatal(err) 
	}
	//Testing End


	switch arg{
	case "key_this_time":
		var key_this_time []byte
		row := db.QueryRow("SELECT key_this_time FROM cards WHERE uid = ?", uid)
		if err != nil {
			log.Fatal(err)
		}


		err = row.Scan(&key_this_time)
		if err != nil {
			log.Printf("Error while scanning row: %v", err)
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
			return 1,nil
		}
	case "TOTP":
		// this need to be fixed since idk what to store in database
		var TOTP string
		row := db.QueryRow("SELECT totp_secret_key FROM cards WHERE uid = ?", uid)
		if err != nil {
			log.Fatal(err)
		}
		err = row.Scan(&TOTP)
		if err != nil {
			log.Fatal(err)
		}
		server_TOTP :=generateTOTPWithSHA256(TOTP, 30)
		server_TOTP_bytes := []byte(server_TOTP)
		if bytes.Equal(input, server_TOTP_bytes){
			return 1,nil
		}

	default:
		fmt.Println("wrong argument")
		return 0,nil
	}
	return 0,nil
}

func database_updater(uid []byte, input []byte, arg string) ( error) {
	db, err := sql.Open("sqlite3", "./testing.db")
	if err != nil {
        return err
    }
	defer db.Close()
	switch arg{
	case "New_Card_Key":
		_, err = db.Exec("UPDATE cards SET key_last_time = key_this_time, key_this_time = ? WHERE uid = ?", input, uid)
		if err != nil {
    	    return err
    	}
	default:
		fmt.Println("Wrong argument")
	}

	return nil
}