---
-- \file    test/t_net_sck_dgram_send.lua
-- \brief   Test assuring sck:send() works in all permutations.
-- \detail  Send data via SOCK_DGRAM sockets. All permutations tested in this
--          suite:
--
--    msg, len  = sck:send( string )
--    msg, len  = sck:send( buffer )
--    msg, len  = sck:send( string, max )
--    msg, len  = sck:send( buffer, max )
--    msg, len  = sck:send( string, address, max )
--    msg, len  = sck:send( buffer, address, max )
--
-- These tests run (semi-)asynchronously.  A UDP server socket is listening while each
-- test connect it's own client to it.  This allows for these tests to run within the
-- same  process.  Each test will restart the loop, connect, assert and stop the
-- loop before moving on to the next test.

local Test      = require( "t.Test" )
local Loop      = require( "t.Loop" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )
local Buffer    = require( "t.Buffer" )
local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr
local config    = t_require( "t_cfg" )


local makeReceiver = function( self, payload, max )
	local f = function( s, msg )
		local msg, rcvd = s.srvSck:recv( )
		assert( rcvd==max   , ("Expected %d bytes but got %d"):format( max, rcvd ) )
		if 0==max then
			assert( msg==nil,  ("Expected `nil` but got\n%s"):format( msg ) )
		else
			assert( msg==payload, ("Expected\n%s\nbut got\n%s"):format( payload, msg ) )
		end
		s.loop:removeHandle( s.srvSck, "read" )
	end
	self.loop:addHandle( self.srvSck, "read", f, self )
end

local makeSender = function( self, func, connect )
	if connect then self.sndSck:connect( self.srvAdr ) end
	self.loop:addHandle( self.sndSck, "write", func, self )
	self.loop:run( )
end


return {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self )
		self.loop  = Loop( )
		self.host  = Interface.default( ).address.ip
		self.port  = config.nonPrivPort
		self.srvSck  = Socket( "udp" )
		self.srvAdr  = self.srvSck:bind( self.host, self.port )
		assert( chkSck( self.srvSck, "IPPROTO_UDP", "AF_INET", "SOCK_DGRAM" ) )
		assert( chkAdr( self.srvAdr, "AF_INET", self.host, self.port ) )
	end,

	afterAll = function( self )
		self.srvSck:close( )
	end,

	beforeEach = function( self )
		self.sndSck  = Socket( "udp" )
		assert( chkSck( self.sndSck, "IPPROTO_UDP", "AF_INET", "SOCK_DGRAM" ) )
	end,

	afterEach = function( self )
		self.sndSck:close( )
	end,

	-- #########################################################################
	-- Actual Test cases

	sendNothing = function( self )
		Test.describe( "snt = sck.send( '' ) -- empty string " )
		local payload = ''
		local sender  = function( s )
			local sent = s.sndSck:send( payload )
			assert( sent==#payload, "Expected `0` but got `" .. sent .. "`" )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, nil, #payload )
		makeSender( self, sender, true )
	end,

	sendString = function( self )
		Test.describe( "snt = sck.send( string )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local sender  = function( s )
			local sent = s.sndSck:send( payload )
			assert( type(sent)=="number", ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==#payload,       ("Expected %d but sent %d bytes"):format( #payload, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload, #payload )
		makeSender( self, sender, true )
	end,

	sendMaxString = function( self )
		Test.describe( "snt = sck.send( string, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.sndSck:send( payload, max )
			assert( type(sent)=="number", ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==max,            ("Expected %d but sent %d bytes"):format( max, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max )
		makeSender( self, sender, true )
	end,

	sendStringTo = function( self )
		Test.describe( "snt = sck.send( string, adr )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local sender  = function( s )
			local sent = s.sndSck:send( payload, s.srvAdr )
			assert( type(sent)=="number", ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==#payload,       ("Expected %d but sent %d bytes"):format( #payload, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload, #payload )
		makeSender( self, sender, false )
	end,

	sendMaxStringTo = function( self )
		Test.describe( "snt = sck.send( string, adr, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.sndSck:send( payload, s.srvAdr, max )
			assert( type(sent)=="number",  ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==max,             ("Expected %d but sent %d bytes"):format( max, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max )
		makeSender( self, sender, false )
	end,

	sendBuffer = function( self )
		Test.describe( "snt = sck.send( buffer )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local buffer  = Buffer( payload )
		local sender  = function( s )
			local sent = s.sndSck:send( buffer )
			assert( type(sent)=="number",  ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==#buffer,         ("Expected %d but sent %d bytes"):format( #buffer, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload, #payload )
		makeSender( self, sender, true )
	end,

	sendMaxBuffer = function( self )
		Test.describe( "snt = sck.send( buffer, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local buffer  = Buffer( payload )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.sndSck:send( buffer, max )
			assert( type(sent)=="number",  ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==max,             ("Expected %d but sent %d bytes"):format( max, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max )
		makeSender( self, sender, true )
	end,

	sendBufferTo = function( self )
		Test.describe( "snt = sck.send( buffer, adr )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local buffer  = Buffer( payload )
		local sender  = function( s )
			local sent = s.sndSck:send( payload, s.srvAdr )
			assert( type(sent)=="number",  ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==#buffer,         ("Expected %d but sent %d bytes"):format( #buffer, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload, #payload )
		makeSender( self, sender, false )
	end,

	sendMaxBufferTo = function( self )
		Test.describe( "snt = sck.send( buffer, adr, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local buffer  = Buffer( payload )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.sndSck:send( buffer, s.srvAdr, max )
			assert( type(sent)=="number",  ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==max,             ("Expected %d but sent %d bytes"):format( max, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max )
		makeSender( self, sender, false )
	end,

	sendMaxTooBigStringAutoTrim = function( self )
		Test.describe( "snt = sck.send( string, max > #string )  autotrim to #string" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local len     = #payload
		local sender  = function( s )
			local sent = s.sndSck:send( payload, len+400 )
			assert( type(sent)=="number",  ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==len,             ("Expected %d but sent %d bytes"):format( len, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload, len )
		makeSender( self, sender, true )
	end,

	sendMaxTooBigBufferAutoTrim = function( self )
		Test.describe( "snt = sck.send( buffer, max > #buffer )  autotrim to #buffer" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local len     = #payload
		local buffer  = Buffer( payload )
		local sender  = function( s )
			local sent = s.sndSck:send( buffer, len+400 )
			assert( type(sent)=="number",  ("Expected `number` but got `%s`"):format( type(sent) ) )
			assert( sent==len,             ("Expected %d but sent %d bytes"):format( len, sent) )
			s.loop:removeHandle( s.sndSck, "write" )
		end
		makeReceiver( self, payload, len )
		makeSender( self, sender, true )
	end,

	sendBadArgumentsFails = function( self )
		Test.describe( "snt = sck.send( Bad Arguments ) FAILS" )
		local payload = string.rep( "TestMessage content for bad arguments -- ", 14 )
		local len, f, eMsg, d, e  = #payload, nil, nil, nil, nil
		local buffer  = Buffer( payload )
		local sender  = function( s )
			-- not using sck:send but sck.send
			f    = function( x ) local snt = x.sndSck.send( payload, "something" ) end
			eMsg = "bad argument #1 to 'send' %(T.Net.Socket expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nbut was\n`%s`"):format( eMsg, e ) )

			f    = function( x ) local snt = x.sndSck:send( payload, "something" ) end
			eMsg = "bad argument #2 to 'send' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nbut was\n`%s`"):format( eMsg, e ) )

			f    = function( x ) local snt = x.sndSck:send( payload, Address(), "something" ) end
			eMsg = "bad argument #3 to 'send' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nbut was\n`%s`"):format( eMsg, e ) )

			s.loop:removeHandle( s.sndSck, "write" )
		end
		--makeReceiver( self, payload, len )
		makeSender( self, sender, true )
	end,
}
