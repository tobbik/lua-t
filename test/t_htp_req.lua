---
-- \file    t_htp_req.lua
-- \brief   Test for the Http Request
local Table    = require't.Table'
local t_map,t_count,t_concat = Table.map, Table.count, table.concat
local Test     = require't.Test'
local Http     = require't.Http'
local Request  = require't.Http.Request'
local Buffer   = require't.Buffer'
local Method, Version, Status = require't.Http.Method', require't.Http.Version', require't.Http.Status'
local format   = string.format

-- Mocking socket providing just a fake send method
local mSck = {
	  threshHold = 40
	, send = function( self, msg )
		local snt = #msg > self.threshHold and math.random( self.threshHold, #msg ) or #msg
		if snt == #msg then
			print( string.format( "SENDING: `%s`-----------------------------------------------------", msg:sub( 1, snt ) ) )
		else
			print( string.format( "SENDING: `%s`", msg:sub( 1, snt ) ) )
		end
		return snt
	end
}

local dummyCb, tReq, tRes

local tests = {
	beforeEach = function( self )
		dummyCb = function( req, res )
			tReq = req
			tRes = res
		end
	end,
	afterEach = function( self )
		tReq, tRes = nil,nil
	end,

	-- Test cases

	-- CONSTRUCTOR TESTS
	test_Constructor = function( self )
		Test.Case.describe( "Http.Request( callback ) creates proper Request" )
		local r = Request( dummyCb )
		assert( r.state   == Request.State.Method, format( "State must be %d but was %d", Request.State.Method, r.state ) )
		assert( r.method  == Method.ILLEGAL, format( "Method must be %d but was %d", Method.ILLEGAL, r.method ) )
		assert( r.version == Version.ILLEGAL, format( "Version must be %d but was %d", Version.ILLEGAL, r.method ) )
	end,

	-- RECEIVE TESTS
	test_ReceiveMethod = function( self )
		Test.Case.describe( "request:recv() partial parses METHOD only" )
		local r = Request( dummyCb )
		local s = Buffer( "GET /index.html?a" ):Segment( )
		r:receive( s )
		assert( r.state   == Request.State.Url, format( "State must be %d but was %d", Request.State.Url, r.state ) )
		assert( r.method  == Method.GET, format( "Method must be %d but was %d", Method.GET, r.method ) )
		assert( r.version == Version.ILLEGAL, format( "Version must be %d but was %d", Version.ILLEGAL, r.method ) )
	end,

	test_ReceiveUrl = function( self )
		Test.Case.describe( "request:recv() partial parses URL without query" )
		local r = Request( dummyCb )
		local u = '/go/wherever/it/wil/be/index.html'
		local s = Buffer( "GET " ..u.." " ):Segment( )
		r:receive( s )
		assert( r.state == Request.State.Version, format( "State must be %d but was %d", Request.State.Version, r.state ) )
		assert( r.url       , "URL must exist" )
		assert( r.url   == u, format( "URL must be %s but was %s", u, r.url ) )
		assert( nil  == r.query, "Query table mustn't exist" )
	end,

	test_ReceiveUrlAndQuery = function( self )
		Test.Case.describe( "request:recv() partial parses URL with query" )
		local r = Request( dummyCb )
		local u = '/go/wherever/it/wil/be/index.html?alpha=1&beta=2&c=gamma&4=delta'
		local s = Buffer( "GET " ..u.." " ):Segment( )
		r:receive( s )
		assert( r.state == Request.State.Version, format( "State must be %d but was %d", Request.State.Version, r.state ) )
		assert( r.url       , "URL must exist" )
		assert( r.url   == u, format( "URL must be %s but was %s", u, r.url ) )
		assert( "table" == type( r.query ), "req.query must be a table" )
		assert( 4       == t_count( r.query ), "req.query must contain 4 elements" )
		for k,v in pairs({alpha='1', beta='2', c=gamma, [4]=delta}) do
			assert( v == r.query[ k ], format( "req.query[ %s ] must be `%s` but was `%s`", k, v, r.query[k] ) )
		end
	end,

	-- ################################### HTTP VERSION
	test_NotReceiveHttpVersion = function( self )
		Test.Case.describe( "Not Trigger HTTP Version parsing before fully recieved line" )
		local r = Request( dummyCb )
		local v = Version[3]                   -- HTTP/1.1
		-- must have 2 more Bytes if \r\n or 1 more if just \n
		local s = Buffer( "GET /go/wherever/it/wil/be/index.html " .. v ..'\r\n' ):Segment( )
		r:receive( s )
		assert( r.version == 0                , format( "Version must be %d but was %d", 0, r.version ) )
		assert( r.state   == Request.State.Version, format( "State must be %d but was %d", Request.State.Version, r.state ) )
	end,

	test_ReceiveHttpVersion = function( self )
		Test.Case.describe( "Full Line triggers parsing of HTTP Version" )
		local r = Request( dummyCb )
		local v = Version[3]                   -- HTTP/1.1
		-- must have 2 more Bytes if \r\n or 1 more if just \n
		local s = Buffer( "GET /go/wherever/it/wil/be/index.html " .. v ..'\r\nAc' ):Segment( )
		r:receive( s )
		assert( r.state   == Request.State.Headers, format( "State must be %d but was %d", Request.State.Headers, r.state ) )
		assert( r.version == Version[ v ]         , format( "Version must be %d but was %d", Version[v], r.version ) )
	end,

	test_ReceiveHttpVersionDoubleLFCR = function( self )
		Test.Case.describe( "HTTP Version trailing double \\r\\n\\r\\n triggers Request DONE" )
		local r = Request( dummyCb )
		local v = Version[3]                   -- HTTP/1.1
		-- must have 2 more Bytes if \r\n or 1 more if just \n
		local s = Buffer( "GET /go/wherever/it/wil/be/index.html " .. v ..'\r\n\r\n' ):Segment( )
		r:receive( s )
		assert( r.state   == Request.State.Done, format( "State must be %d but was %d", Request.State.Done, r.state ) )
		assert( r.version == Version[ v ]      , format( "Version must be %d but was %d", Version[v], r.version ) )
	end,

	test_ReceiveHttpVersionDoubleLF = function( self )
		Test.Case.describe( "HTTP Version trailing double \\n\\n triggers Request DONE" )
		local r = Request( dummyCb )
		local v = Version[3]                   -- HTTP/1.1
		-- must have 2 more Bytes if \r\n or 1 more if just \n
		local s = Buffer( "GET /go/wherever/it/wil/be/index.html " .. v ..'\n\n' ):Segment( )
		r:receive( s )
		assert( r.state   == Request.State.Done, format( "State must be %d but was %d", Request.State.Done, r.state ) )
		assert( r.version == Version[ v ]      , format( "Version must be %d but was %d", Version[v], r.version ) )
	end,


	-- ################################# HTTP Request Headers
	test_ReceiveHeaders = function( self )
		Test.Case.describe( "request:recv() partial parses Headers" )
		local r = Request( dummyCb )
		-- taken from Wikipedia examples
		local t = {
			  [ 'Accept' ]                         = "text/plain"
			, [ 'Accept-Charset' ]                 = "utf-8"
			, [ 'Accept-Encoding' ]                = "gzip, deflate"
			, [ 'Accept-Language' ]                = "en-US"
			, [ 'Accept-Datetime' ]                = "Thu, 31 May 2007 20:35:00 GMT"
			, [ 'Access-Control-Request-Method' ]  = "GET"
			, [ 'Access-Control-Request-Headers' ] = "X-PINGOTHER, Content-Type"
			, [ 'Authorization' ]                  = "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
			, [ 'Cache-Control' ]                  = "no-cache"
			, [ 'Connection' ]                     = "keep-alive"
			, [ 'Cookie' ]                         = "$Version=1; Skin=new;"
			, [ 'Content-Length' ]                 = "348"
			, [ 'Content-MD5' ]                    = "Q2hlY2sgSW50ZWdyaXR5IQ=="
			, [ 'Content-Type' ]                   = "application/x-www-form-urlencoded"
			, [ 'Date' ]                           = "Tue, 15 Nov 1994 08:12:31 GMT"
			, [ 'Expect' ]                         = "100-continue"
			, [ 'Forwarded' ]                      = "for=192.0.2.60;proto=http;\r\n" ..
			                                         " by=203.0.113.43"
			, [ 'From' ]                           = "user@example.com"
			, [ 'Host' ]                           = "en.wikipedia.org:8080"
			, [ 'If-Match' ]                       = '"737060cd8c284d8af7ad3082f209582d"'
			, [ 'If-Modified-Since' ]              = "Sat, 29 Oct 1994 19:43:31 GMT"
			, [ 'If-None-Match' ]                  = '"737060cd8c284d8af7ad3082f209582d"'
			, [ 'If-Range' ]                       = '"737060cd8c284d8af7ad3082f209582d"'
			, [ 'If-Unmodified-Since' ]            = "Sat, 29 Oct 1994 19:43:31 GMT"
			, [ 'Max-Forwards' ]                   = "10"
			, [ 'Origin' ]                         = "http://www.example-social-network.com"
			, [ 'Pragma' ]                         = "no-cache"
			, [ 'Proxy-Authorization' ]            = "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
			, [ 'Range' ]                          = "bytes=500-999"
			, [ 'Referer' ]                        = "http://en.wikipedia.org/wiki/Main_Page"
			, [ 'TE' ]                             = "trailers, deflate"
			, [ 'User-Agent' ]                     = "Mozilla/5.0 (X11; Linux x86_64; rv:12.0)\r\n" ..
			                                         "  Gecko/20100101 Firefox/12.0"
			, [ 'Upgrade' ]                        = "HTTPS/1.3, IRC/6.9, RTA/x11, websocket"
			, [ 'Via' ]                            = "1.0 fred, 1.1 example.com (Apache/1.1)"
			, [ 'Warning' ]                        = "199 Miscellaneous warning"
		}
		local h = ''
		for k,v in pairs(t) do h = h .. k ..': '..v.. '\r\n' end
		local s = Buffer( "GET /go/index.html HTTP/1.1\r\n" .. h .. '\r\n' ):Segment( )
		r:receive( s )
		for k,v in pairs( t ) do
			assert( v == r.headers[k], format( "req.headers[ %s ] must be `%s` but was `%s`", k, v, r.headers[k] ) )
		end
		assert( r.state == Request.State.Body, format( "State must be %d but was %d", Request.State.Body, r.state ) )
	end,

	test_ReceiveHeadersCorrectCasing = function( self )
		Test.Case.describe( "request:recv() correct Headers Key Casing" )
		local r = Request( dummyCb )
		-- taken from Wikipedia examples
		local t = {
			  [ 'Accept' ]                         = "text/plain"
			, [ 'Accept-Charset' ]                 = "utf-8"
			, [ 'Accept-Encoding' ]                = "gzip, deflate"
			, [ 'Accept-Language' ]                = "en-US"
			, [ 'Accept-Datetime' ]                = "Thu, 31 May 2007 20:35:00 GMT"
			, [ 'Access-Control-Request-Method' ]  = "GET"
			, [ 'Access-Control-Request-Headers' ] = "X-PINGOTHER, Content-Type"
			, [ 'Authorization' ]                  = "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
			, [ 'Cache-Control' ]                  = "no-cache"
			, [ 'Connection' ]                     = "keep-alive"
			, [ 'Cookie' ]                         = "$Version=1; Skin=new;"
			, [ 'Content-Length' ]                 = "348"
			, [ 'Content-MD5' ]                    = "Q2hlY2sgSW50ZWdyaXR5IQ=="
			, [ 'Content-Type' ]                   = "application/x-www-form-urlencoded"
			, [ 'Date' ]                           = "Tue, 15 Nov 1994 08:12:31 GMT"
			, [ 'Expect' ]                         = "100-continue"
			, [ 'Forwarded' ]                      = "for=192.0.2.60;proto=http;\r\n" ..
			                                         " by=203.0.113.43"
			, [ 'From' ]                           = "user@example.com"
			, [ 'Host' ]                           = "en.wikipedia.org:8080"
			, [ 'If-Match' ]                       = '"737060cd8c284d8af7ad3082f209582d"'
			, [ 'If-Modified-Since' ]              = "Sat, 29 Oct 1994 19:43:31 GMT"
			, [ 'If-None-Match' ]                  = '"737060cd8c284d8af7ad3082f209582d"'
			, [ 'If-Range' ]                       = '"737060cd8c284d8af7ad3082f209582d"'
			, [ 'If-Unmodified-Since' ]            = "Sat, 29 Oct 1994 19:43:31 GMT"
			, [ 'Max-Forwards' ]                   = "10"
			, [ 'Origin' ]                         = "http://www.example-social-network.com"
			, [ 'Pragma' ]                         = "no-cache"
			, [ 'Proxy-Authorization' ]            = "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="
			, [ 'Range' ]                          = "bytes=500-999"
			, [ 'Referer' ]                        = "http://en.wikipedia.org/wiki/Main_Page"
			, [ 'TE' ]                             = "trailers, deflate"
			, [ 'User-Agent' ]                     = "Mozilla/5.0 (X11; Linux x86_64; rv:12.0)\r\n" ..
			                                         "  Gecko/20100101 Firefox/12.0"
			, [ 'Upgrade' ]                        = "HTTPS/1.3, IRC/6.9, RTA/x11, websocket"
			, [ 'Via' ]                            = "1.0 fred, 1.1 example.com (Apache/1.1)"
			, [ 'Warning' ]                        = "199 Miscellaneous warning"
		}
		local h = ''
		for k,v in pairs(t) do h = h ..string.lower( k )..': '..v.. '\r\n' end -- string.lower() for bad casing
		local s = Buffer( "GET /go/index.html HTTP/1.1\r\n" .. h .. '\r\n' ):Segment( )
		r:receive( s )
		for k,v in pairs( t ) do
			 assert( v == r.headers[k], format( "req.headers[ %s ] must be `%s` but was `%s`", k, v, r.headers[k] ) )
		end
		assert( r.state == Request.State.Body, format( "State must be %d but was %d", Request.State.Body, r.state ) )
	end,


	test_ReceiveBadHeaders = function( self )
		Test.Case.describe( "request:recv() Unparsable Headers will be enumerated" )
		local r = Request( dummyCb )
		local t = {
			  [ 'Content-Length' ] = "12345"
			, [ 'TE' ]             = "trailers, deflate"
			, [ 'User-Agent' ]     = "Mozilla/5.0 (X11; Linux x86_64; rv:12.0)\r\n" ..
			                         "Gecko/20100101 Firefox/12.0"
			, [ 'Upgrade' ]        = "HTTPS/1.3, IRC/6.9, RTA/x11, websocket"
		}
		local h = ''
		for k,v in pairs(t) do  h = h ..k..': '..v.. '\r\n' end
		local s = Buffer( "GET /go/index.html HTTP/1.1\r\n" .. h .. '\r\n' ):Segment( )
		r:receive( s )
		local t1 = {
			  [ 'Content-Length' ] = "12345"
			, [ 'TE' ]             = "trailers, deflate"
			, [ 'User-Agent' ]     = "Mozilla/5.0 (X11; Linux x86_64; rv:12.0)"
			, [ 0 ]                = "Gecko/20100101 Firefox/12.0" -- unparsable headers will be enumerated
			, [ 'Upgrade' ]        = "HTTPS/1.3, IRC/6.9, RTA/x11, websocket"
		}
		for k,v in pairs( t1 ) do
			assert( v == r.headers[ k ], format( "req.headers[ %s ] must be `%s` but was `%s`", k, v, r.headers[k] ) )
		end
		assert( r.state   == Request.State.Body, format( "State must be %d but was %d", Request.State.Body, r.state ) )
	end,

	-- #################################### Headers content parsing
	test_HeaderContentLength = function( self )
		Test.Case.describe( "Content-Length Header gets parsed" )
		local r = Request( dummyCb )
		local s = Buffer( "GET /go/index.html HTTP/1.1\r\nContent-Length: 12345\r\n\r\n" ):Segment( )
		r:receive( s )
		assert( 12345 == r.contentLength, format( "req.contentLength must be `%d` but was `%s`", 12345, r.contentLength ) )
	end,

	test_HeaderNoContentLength = function( self )
		Test.Case.describe( "No Content-Length finishes as Request.State.Done" )
		local r = Request( dummyCb )
		-- taken from Wikipedia examples
		local t = {
			  [ 'Accept' ]                         = "text/plain"
			, [ 'Accept-Charset' ]                 = "utf-8"
			, [ 'Accept-Encoding' ]                = "gzip, deflate"
			, [ 'Accept-Language' ]                = "en-US"
			, [ 'Accept-Datetime' ]                = "Thu, 31 May 2007 20:35:00 GMT"
		}
		local h = ''
		for k,v in pairs(t) do h = h .. k ..': '..v.. '\r\n' end
		local s = Buffer( "GET /go/index.html HTTP/1.1\r\n" .. h .. '\r\n' ):Segment( )
		r:receive( s )
		assert( nil == r.contentLength, format( "req.contentLength must be `nil` but was `%s`", r.contentLength ) )
		assert( r.state == Request.State.Done, format( "State must be %d but was %d", Request.State.Done, r.state ) )
	end,

	-- #################################### Continous parsing
	test_ContinuedUrl = function( self )
		Test.Case.describe( "request:recv() partial URL and finish parsing" )
		local r      = Request( dummyCb )
		local u1, u2 = '/go/wherever/it/wil', 'l/be/index.html'
		local u      = u1 .. u2
		local v      = Version[3] -- HTTP/1.1
		local p1, p2 = 'GET ' .. u1, u2 .. ' ' ..v.. '\r\nAc'
		local s1     = Buffer( p1 ):Segment( )
		r:receive( s1 )
		assert( r.state == Request.State.Url, format( "State must be %d but was %d", Request.State.Url, r.state ) )
		assert( not r.url, "URL mustn't exist" )
		assert( r.buf:read() == p1:sub(4), format("r.buf shall be `%s` but was `%s", r.buf:read(), p1:sub(4)) )
		local s2 = Buffer( p2 ):Segment( )
		r:receive( s2 )
		assert( r.url     == u, format( "URL must be `%s` but was `%s`", u, r.url ) )
		assert( r.version == Version[v], format( "HTTP version must be `%s` but was `%s`", Version[v], r.version ) )
		assert( r.state == Request.State.Headers, format( "State must be %d but was %d", Request.State.Headers, r.state ) )
	end,


}

return Test( tests )
