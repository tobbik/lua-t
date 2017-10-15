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

Test      = require( "t.Test" )
Timer     = require( "t.Time" )
Loop      = require( "t.Loop" )
Socket    = require( "t.Net.Socket" )
Address   = require( "t.Net.Address" )
Interface = require( "t.Net.Interface" )
Buffer    = require( "t.Buffer" )
T         = require( "t" )
t_assert,t_require  = require't'.assert, require't'.require
asrtHlp   = t_require( "assertHelper" )


local makeReceiver = function( self, payload, max, done )
	local f = function( s, msg )
		local msg, rcvd = s.sSck:recv( )
		t_assert( rcvd==max   , "Expected %d bytes but got %d", max, rcvd )
		if 0==max then
			t_assert( msg==nil,  "Expected `nil` but got\n%s", msg )
		else
			t_assert( msg==payload, "Expected\n%s\nbut got\n%s", payload, msg )
		end
		s.loop:removeHandle( s.sSck, "read" )
		done( )
	end
	self.loop:addHandle( self.sSck, "read", f, self )
end

local makeSender = function( self, func, connect )
	if connect then self.cSck:connect( self.sAdr ) end
	self.loop:addHandle( self.cSck, "write", func, self )
end


local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop  = Loop( 20 )
		self.host  = Interface( "default" ).AF_INET.address.ip
		self.port  = 8000
		self.sSck  = Socket( "udp" )
		self.sAdr  = self.sSck:bind( self.host, self.port )
		asrtHlp.Socket( self.sSck, "udp", "AF_INET", "SOCK_DGRAM" )
		asrtHlp.Address( self.sAdr, "AF_INET", self.host, self.port )
		done()
	end,

	afterAll = function( self, done )
		self.sSck:close( )
		done()
	end,

	beforeEach_cb = function( self, done )
		self.cSck  = Socket( "udp" )
		asrtHlp.Socket( self.cSck, "udp", "AF_INET", "SOCK_DGRAM" )

		self.loop:addTimer( Timer( 1 ), done )
		-- loop:run() blocks further execution until the function on the loop
		-- runs the afterEach_cb and releases the block forcing all tests to be
		-- executed sequentially
		self.loop:run()
	end,

	afterEach_cb = function( self, done )
		self.loop:stop( )
		self.cSck:close( )
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases

	test_cb_sendNothing = function( self, done )
		Test.Case.describe( "snt = sck.send( '' ) -- empty string " )
		local payload = ''
		local sender  = function( s )
			local sent = s.cSck:send( payload )
			assert( sent==#payload, "Expected `0` but got `" .. sent .. "`" )
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, nil, #payload, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendString = function( self, done )
		Test.Case.describe( "snt = sck.send( string )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local sender  = function( s )
			local sent = s.cSck:send( payload )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==#payload,        "Expected %d but sent %d bytes", #payload, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload, #payload, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendMaxString = function( self, done )
		Test.Case.describe( "snt = sck.send( string, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.cSck:send( payload, max )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==max,         "Expected %d but sent %d bytes", max, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendStringTo = function( self, done )
		Test.Case.describe( "snt = sck.send( string, adr )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local sender  = function( s )
			local sent = s.cSck:send( payload, s.sAdr )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==#payload,        "Expected %d but sent %d bytes", #payload, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload, #payload, done )
		makeSender( self, sender, false )
	end,

	test_cb_sendMaxStringTo = function( self, done )
		Test.Case.describe( "snt = sck.send( string, adr, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.cSck:send( payload, s.sAdr, max )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==max,         "Expected %d but sent %d bytes", max, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max, done )
		makeSender( self, sender, false )
	end,

	test_cb_sendBuffer = function( self, done )
		Test.Case.describe( "snt = sck.send( buffer )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local buffer  = Buffer( payload )
		local sender  = function( s )
			local sent = s.cSck:send( buffer )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==#buffer,        "Expected %d but sent %d bytes", #buffer, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload, #payload, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendMaxBuffer = function( self, done )
		Test.Case.describe( "snt = sck.send( buffer, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local buffer  = Buffer( payload )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.cSck:send( buffer, max )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==max,         "Expected %d but sent %d bytes", max, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendBufferTo = function( self, done )
		Test.Case.describe( "snt = sck.send( buffer, adr )" )
		local payload = string.rep( "TestMessage content for sending full message -- ", 14 )
		local buffer  = Buffer( payload )
		local sender  = function( s )
			local sent = s.cSck:send( payload, s.sAdr )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==#buffer,        "Expected %d but sent %d bytes", #buffer, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload, #payload, done )
		makeSender( self, sender, false )
	end,

	test_cb_sendMaxBufferTo = function( self, done )
		Test.Case.describe( "snt = sck.send( buffer, adr, max )" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local buffer  = Buffer( payload )
		local max     = #payload//2
		local sender  = function( s )
			local sent = s.cSck:send( buffer, s.sAdr, max )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==max,         "Expected %d but sent %d bytes", max, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload:sub(1,max), max, done )
		makeSender( self, sender, false )
	end,

	test_cb_sendMaxTooBigStringAutoTrim = function( self, done )
		Test.Case.describe( "snt = sck.send( string, max > #string )  autotrim to #string" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local len     = #payload
		local sender  = function( s )
			local sent = s.cSck:send( payload, len+400 )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==len,         "Expected %d but sent %d bytes", len, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload, len, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendMaxTooBigBufferAutoTrim = function( self, done )
		Test.Case.describe( "snt = sck.send( buffer, max > #buffer )  autotrim to #buffer" )
		local payload = string.rep( "TestMessage content for sending sized message -- ", 14 )
		local len     = #payload
		local buffer  = Buffer( payload )
		local sender  = function( s )
			local sent = s.cSck:send( buffer, len+400 )
			t_assert( type(sent)=="number",  "Expected `number` but got `%s`", type(sent) )
			t_assert( sent==len,         "Expected %d but sent %d bytes", len, sent)
			s.loop:removeHandle( s.cSck, "write" )
		end
		makeReceiver( self, payload, len, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendBadArgumentsFails = function( self, done )
		Test.Case.describe( "snt = sck.send( Bad Arguments ) FAILS" )
		local payload = string.rep( "TestMessage content for bad arguments -- ", 14 )
		local len, f, eMsg, d, e  = #payload, nil, nil, nil, nil
		local buffer  = Buffer( payload )
		local sender  = function( s )
			-- not using sck:send but sck.send
			f    = function( x ) local snt = x.cSck.send( payload, "something" ) end
			eMsg = "bad argument #1 to 'send' %(T.Net.Socket expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			t_assert( e:match( eMsg ), "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e )

			f    = function( x ) local snt = x.cSck:send( payload, "something" ) end
			eMsg = "bad argument #2 to 'send' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			t_assert( e:match( eMsg ), "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e )

			f    = function( x ) local snt = x.cSck:send( payload, Address(), "something" ) end
			eMsg = "bad argument #3 to 'send' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			t_assert( e:match( eMsg ), "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e )

			s.loop:removeHandle( s.cSck, "write" )
			done()
		end
		--makeReceiver( self, payload, len, done )
		makeSender( self, sender, true )
	end,
}

-- t = Test( tests )
-- t()
-- print( t )

return Test( tests )
