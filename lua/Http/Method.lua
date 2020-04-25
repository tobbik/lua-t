local Table = require('t.Table')

local methods = {
	  [ 0  ]  = "ILLEGAL"
	, [ 1  ]  = "CONNECT"
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

-- reverse lookup
for k,v in pairs( Table.clone(methods) ) do
	methods[ v ] = k
end

return methods
