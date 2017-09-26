---
-- \file    t_htp_rsp.lua
-- \brief   Test for the Http Response
local Test     = require't.Test'
local Http     = require't.Http'
local Response = require't.Http.Response'
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

local makeResonse = function()
	return Response( {}, 1, true, 3 )
end

local tests = {
	-- Test cases

	-- CONSTRUCTOR TESTS
	test_Constructor = function( self )
		Test.Case.describe( "Http.Response( id, keepAlive, version ) creates proper Response" )
		local r = makeResonse( )
		assert( r.state == Response.State.Zero, format( "State must be %d but was %d", Response.State.Zero, r.state ) )
		assert( r.keepAlive, "Response must be using keepAlive" )
		assert( r.id == 1, "Response.id must be 123 but was " .. r.id )
		assert( r.version == 3, format( "Http.version must `%s` but was `%s`", r.version, 3 ) )
	end,

	--  ##############              WRITE HEAD
	test_Writehead = function( self )
		Test.Case.describe( "response:writeHead( status ) HeadBuffer" )
		local r = makeResonse( )
		r:writeHead( 200 )
		assert( r.state   == Response.State.HeadDone, format( "State must be %d but was %d", Response.State.HeadDone, r.state ) )
		assert( r.chunked, "Response must be chunked" )
		assert( #r.buf == 1, "Response Buffer must be length 1 but was " .. #r.buf )
		assert( r.buf[1]:match("\r\nTransfer%-Encoding: chunked\r\n"), "Response Buffer should match 'Transfer-Encoding: chunked'" )
		assert( r.buf[1]:match("\r\nConnection: Keep%-Alive\r\n"), "Response Buffer should match 'Connection: Keep-Alive'" )
		local dtStr = "Date: " .. os.date( "%a, %d %b %Y %H:%M:", os.time() )
		assert( r.buf[1]:match("\r\n" .. dtStr ), format( "Response Buffer should match '%s' but found '%s'", dtStr, r.buf[1] ) )
	end,

	test_WriteheadLength = function( self )
		Test.Case.describe( "response:writeHead( status, length ) HeadBuffer" )
		local r = makeResonse( )
		local l = 500
		r:writeHead( 200, l )
		assert( not r.chunked, "Response must not be chunked" )
		assert( not r.buf[1]:match("\r\nTransfer%-Encoding: chunked\r\n"), "Response Buffer should not match 'Transfer-Encoding: chunked'" )
		assert( r.buf[1]:match("\r\nContent%-Length%: " ..l.. "\r\n"), "Response Buffer should match 'Content-Length: " ..l.. "' but found `%s`", l, r.buf[1] )
	end,

	test_WriteheadStatusCode = function( self )
		Test.Case.describe( "response:writeHead( status ) Fetches correct status message" )
		for cde,msg in pairs( Status ) do
			local r = makeResonse( )
			r:writeHead( cde )
			local term = format( "^%s %d %s\r\n", Version[3], cde, msg ):gsub( '%-', '%%-' )
			assert( r.buf[1]:match( term), format( "Response Buffer should match '%s' but found `%s`", term, r.buf[1] ) )
		end
	end,

	test_WriteheadHeader = function( self )
		Test.Case.describe( "response:writeHead( status, headers ) HeadBuffer" )
		local r = makeResonse( )
		local l = 500
		r:writeHead( 200, {['Content-Disposition']='attachment; filename="fname.ext"', ['ETag']='"737060cd8c284d8af7ad3082f209582d"'} )
		local rbuf = table.concat( r.buf )
		assert( rbuf:match('\r\nContent%-Disposition: attachment; filename="fname.ext"\r\n'),
				format( "Response Buffer should match '%s' but found `%s`", 'Content-Disposition: attachment; filename="fname.ext"', rbuf ) )
		assert( rbuf:match('\r\nETag: "737060cd8c284d8af7ad3082f209582d"\r\n'),
				format( "Response Buffer should match '%s' but found `%s`", 'ETag: "737060cd8c284d8af7ad3082f209582d"', rbuf ) )
	end,

	--  ##############               RESPONSE finish( )
	test_FinishFinal = function( self )
		Test.Case.describe( "response:finish( Message ) Sends content with length when called withoud writehead() or write() before" )
		local r = makeResonse( )
		local payload = '{"random":"data of the payload", "is":true, "just":"A simple JSON content"}'
		r:finish( payload )
		assert( not r.chunked, "Response must not be chunked" )
		assert( r.buf[1]:match("\r\nContent%-Length%: " ..#payload.. "\r\n"),
			format("Response Buffer should match 'Content-Length: %d' but found `%s`", #payload, r.buf[1] ) )
		assert( r.buf[2]:match(payload), format( "Response Buffer should match `%s` but found `%s`", payload, r.buf[2]) )
	end,


}

return Test( tests )
