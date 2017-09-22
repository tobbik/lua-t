---
-- \file    t_htp_req.lua
-- \brief   Test for the Http Request
local Table    = require't.Table'
local t_map,t_concat = Table.map, table.concat
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
		local b = Buffer( "GET /index.html?a" )
		r:receive( b:Segment() )
		assert( r.state   == Request.State.Url, format( "State must be %d but was %d", Request.State.Url, r.state ) )
		assert( r.method  == Method.GET, format( "Method must be %d but was %d", Method.GET, r.method ) )
		assert( r.version == Version.ILLEGAL, format( "Version must be %d but was %d", Version.ILLEGAL, r.method ) )
	end,

	test_ReceiveUrl = function( self )
		Test.Case.describe( "request:recv() partial parses URL without query" )
		local r = Request( dummyCb )
		local u = '/go/wherever/it/wil/be/index.html'
		local b = Buffer( "GET " ..u.." " )
		r:receive( b:Segment() )
		assert( r.state == Request.State.Version, format( "State must be %d but was %d", Request.State.Version, r.state ) )
		assert( r.url       , "URL must exist" )
		assert( r.url   == u, format( "URL must be %s but was %s", u, r.url ) )
		assert( nil  == r.query, "Query table mustn't exist" )
	end,

	test_ReceiveUrlAndQuery = function( self )
		Test.Case.describe( "request:recv() partial parses URL with query" )
		local r = Request( dummyCb )
		local u = '/go/wherever/it/wil/be/index.html?alpha=1&beta=2&c=gamma&4=delta'
		local b = Buffer( "GET " ..u.." " )
		r:receive( b:Segment() )
		assert( r.state == Request.State.Version, format( "State must be %d but was %d", Request.State.Version, r.state ) )
		assert( r.url       , "URL must exist" )
		assert( r.url   == u, format( "URL must be %s but was %s", u, r.url ) )
		assert( "table" == type( r.query ), "req.query must be a table" )
		assert( 4       == Table.count( r.query ), "req.query must contain 4 elements" )
		for k,v in pairs({alpha='1', beta='2', c=gamma, [4]=delta}) do
			assert( v == r.query[ k ], format( "req.query[ %s ] must be `%s` but was `%s`", k, v, r.query[k] ) )
		end
	end,

	test_ReceiveHttpVersion = function( self )
		Test.Case.describe( "request:recv() partial parses HTTP Version" )
		local r = Request( dummyCb )
		local v = Version[3] -- HTTP/1.1
		local b = Buffer( "GET /go/wherever/it/wil/be/index.html " .. v ..'\r\n' )
		r:receive( b:Segment() )
		assert( r.state   == Request.State.Headers, format( "State must be %d but was %d", Request.State.Headers, r.state ) )
		assert( r.version == Version[ v ]         , format( "Version must be %d but was %d", Version[v], r.version ) )
		--assert( r.method, "r.method must exist" )
		assert( r.url, "r.url must exist" )
	end,

	test_ReceiveHeaders = function( self )
		Test.Case.describe( "request:recv() partial parses Headers" )
		local r = Request( dummyCb )
		local t = {
			HeaderOne   = 'ValueOne',
			HeaderTwo   = 'ValueTwo;\r\n ValueTwo Continuation',
			HeaderThree = 'ValueThree' }
		local h = ''
		t_map( t, function(v,k) h = h ..k..': '..v.. '\r\n' end )
		local b = Buffer( "GET /go/index.html HTTP/1.1\r\n" .. h .. '\r\n' )
		r:receive( b:Segment() )
		for k,v in pairs( t ) do
			assert( v == r.headers[ k ], format( "req.headers[ %s ] must be `%s` but was `%s`", k, v, r.headers[k] ) )
		end
		assert( r.state   == Request.State.Body, format( "State must be %d but was %d", Request.State.Body, r.state ) )
	end,

}

return Test( tests )
