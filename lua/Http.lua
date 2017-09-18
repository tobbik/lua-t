local Http      = require( "t.htp" )
Http.Connection = require( "t.Http.Connection" )
Http.Server     = require( "t.Http.Server" )
Http.Stream     = require( "t.Http.Stream" )
--Http.Websocket  = require( "t.Http.WebSocket" )

Http.Version    = {
	  [ 1 ]   = "HTTP/0.9"
	, [ 2 ]   = "HTTP/1.0"
	, [ 3 ]   = "HTTP/1.1"
	, [ 4 ]   = "HTTP/2"
}


Http.Method     = {
	  [ 1  ]  = "CONNECT"
	, [ 2  ]  = "CHECKOUT"
	, [ 3  ]  = "COPY"
	, [ 4  ]  = "DELETE"
	, [ 5  ]  = "GET"
	, [ 6  ]  = "HEAD"
	, [ 7  ]  = "LOCK"
	, [ 8  ]  = "MKCOL"
	, [ 9  ]  = "MKACTIVITY"
	, [ 10 ]  = "MKCALENDAR"
	, [ 11 ]  = "M-SEARCH"
	, [ 12 ]  = "MERGE"
	, [ 13 ]  = "MOVE"
	, [ 14 ]  = "NOTIFY"
	, [ 15 ]  = "OPTIONS"
	, [ 16 ]  = "POST"
	, [ 17 ]  = "PUT"
	, [ 18 ]  = "PATCH"
	, [ 19 ]  = "PURGE"
	, [ 20 ]  = "PROPFIND"
	, [ 21 ]  = "PROPPATCH"
	, [ 22 ]  = "REPORT"
	, [ 23 ]  = "SUBSCRIBE"
	, [ 24 ]  = "SEARCH"
	, [ 25 ]  = "TRACE"
	, [ 26 ]  = "UNLOCK"
	, [ 27 ]  = "UNSUBSCRIBE"
}


Http.Status     = {
	  [ 100 ] = "Continue"
	, [ 101 ] = "Switching Protocols"
	, [ 200 ] = "OK"
	, [ 201 ] = "Created"
	, [ 202 ] = "Accepted"
	, [ 203 ] = "Non-Authoritative Information"
	, [ 204 ] = "No Content"
	, [ 205 ] = "Reset Content"
	, [ 206 ] = "Partial Content"
	, [ 300 ] = "Multiple Choices"
	, [ 301 ] = "Moved Permanently"
	, [ 302 ] = "Found"
	, [ 303 ] = "See Other"
	, [ 304 ] = "Not Modified"
	, [ 305 ] = "Use Proxy"
	, [ 307 ] = "Temporary Redirect"
	, [ 400 ] = "Bad Request"
	, [ 401 ] = "Unauthorized"
	, [ 402 ] = "Payment Required"
	, [ 403 ] = "Forbidden"
	, [ 404 ] = "Not Found"
	, [ 405 ] = "Method Not Allowed"
	, [ 406 ] = "Not Acceptable"
	, [ 407 ] = "Proxy Authentication Required"
	, [ 408 ] = "Request Timeout"
	, [ 409 ] = "Conflict"
	, [ 410 ] = "Gone"
	, [ 411 ] = "Length Required"
	, [ 412 ] = "Precondition Failed"
	, [ 413 ] = "Payload Too Large"
	, [ 414 ] = "URI Too Long"
	, [ 415 ] = "Unsupported Media Type"
	, [ 416 ] = "Range Not Satisfiable"
	, [ 417 ] = "Expectation Failed"
	, [ 426 ] = "Upgrade Required"
	, [ 500 ] = "Internal Server Error"
	, [ 501 ] = "Not Implemented"
	, [ 502 ] = "Bad Gateway"
	, [ 503 ] = "Service Unavailable"
	, [ 504 ] = "Gateway Timeout"
	, [ 505 ] = "HTTP Version Not Supported"
}


return Http
