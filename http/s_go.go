package main

import (
	"log"
	"fmt"
	"net"
	"net/http"
	"regexp"
	"strings"
	"strconv"
	"os"
	"math/rand"
	"time"
)

const (
	hostname = "0.0.0.0"
)

const letterBytes = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

func RandStringBytes(n int) string {
	b := make([]byte, n)
	for i := range b {
		b[i] = letterBytes[rand.Intn(len(letterBytes))]
	}
	return string(b)
}

func getUsers( i int ) map[string] string {
	users    := make( map[string] string )
	for x := 1; x < i; x++ {
		users[ RandStringBytes(rand.Intn(12 - 6) + 6) ] = RandStringBytes(rand.Intn(12 - 6) + 6)
	}
	return users
}

var (
	re       = regexp.MustCompile( "[!@#$%^&*]" )
	users    = getUsers( 5000 )
	rotLot   = make( map[rune]   rune )
	r_cnt    = 0
	payload  = strings.Repeat("This is a simple dummy load that is meant to generate some load", 10 )
)

func rot47map( c rune ) rune {
	return '!' + (c - '!' + 47) % 94
}

func rot47( pw string ) string {
	return strings.Map( rot47map, pw )
}

func handlerNew(w http.ResponseWriter, req *http.Request) {
	queryValues := req.URL.Query()

	uname,_ := queryValues[ "username" ]
	pword,_ := queryValues[ "password" ]

	var username string = strings.Join( uname, "" )
	var password string = strings.Join( pword, "" )

	username = re.ReplaceAllString( username, "" )
	_, haveUser  := users[ username ]
	w.Header( ).Set( "Keep-Alive", "timeout=5" )

	//fmt.Printf( "%s   %s   %s", username, password, users[ username ] )
	if (""==username || ""==password || haveUser) {
		w.WriteHeader( http.StatusBadRequest )
		var reason = "User already exists"
		if (! haveUser ) {
			reason = "Insufficient arguments"
		}
		w.Write( []byte( fmt.Sprintf( "Creating a new user failed -> %s\n", reason ) ) )
	} else {
		users[ username ] = rot47( password );
		fmt.Printf( "Created User -> %s:%s\n", username, users[ username ] )
		w.WriteHeader( http.StatusOK )
		w.Header().Set( "Content-Type", "text/plain" )
		w.Write( []byte( fmt.Sprintf( "Created new user `%s`\n", username ) ) );
	}
	return
}

func handlerAuth( w http.ResponseWriter, req *http.Request ) {
	queryValues := req.URL.Query()

	uname,_ := queryValues[ "username" ]
	pword,_ := queryValues[ "password" ]

	var username string = strings.Join( uname, "" )
	var password string = strings.Join( pword, "" )
	w.Header( ).Set( "Keep-Alive", "timeout=5" )
	w.Header().Set( "Connection", "keep-alive" )

	username = re.ReplaceAllString( username, "" )
	_, haveUser  := users[ username ]

	if (""==username || ""==password || ! haveUser) {
		w.WriteHeader( http.StatusBadRequest )
		w.Write( []byte( "Unknown User" ) )
	} else {
		if (users[ username ] == rot47( password )) {
			r_cnt++;
			w.WriteHeader( http.StatusOK )
			w.Header( ).Set( "Content-Type", "text/plain" )
			w.Write( []byte( fmt.Sprintf( "%7d This user was authorized at %d\n", r_cnt, time.Now().UnixNano() / 1e6 ) ) );
		} else {
			w.WriteHeader( http.StatusBadRequest )
			w.Write( []byte( "Authorization failed!" ) );
		}
	}
	return
}

func handlerMulti( w http.ResponseWriter, req *http.Request ) {
	queryValues := req.URL.Query()

	mplier,_ := queryValues[ "multiplier" ]
	if multiplier, err := strconv.Atoi( strings.Join( mplier, "" ) ); err == nil {
		w.WriteHeader( http.StatusOK )
		w.Header( ).Set( "Content-Type", "text/plain" )
		w.Write( []byte( strings.Repeat(payload, multiplier ) ) );
	} else {
		w.WriteHeader( http.StatusBadRequest )
		w.Write( []byte( "Server error bad arguments" ) );
	}
	return
}

func handler404( w http.ResponseWriter, req *http.Request ) {
	w.WriteHeader( http.StatusNotFound )
	w.Write( []byte( "There is nothing to do here\r\n" ) );
	return
}

func main() {
	port  := "8000"
	if 2==len( os.Args ) {
		port = os.Args[ 1 ]
	}
	http.HandleFunc( "/newUser", handlerNew )
	http.HandleFunc( "/auth", handlerAuth )
	http.HandleFunc( "/multi", handlerMulti )
	http.HandleFunc( "/", handler404 )
	log.Printf( "Server running at http://%s:%s", hostname, port )
	err := http.ListenAndServe( net.JoinHostPort( hostname, port ), nil )
	log.Fatal( err )
}
