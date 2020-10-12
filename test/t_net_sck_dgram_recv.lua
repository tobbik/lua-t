---
-- \file    test/t_net_sck_dgram_recv.lua
-- \brief   Test assuring sck:recv() works in all permutations.
-- \detail  Receive data via SOCK_DGRAM sockets. All permutations
--          tested in this suite:
--
--    msg, len  = sck:recv( )
--    msg, len  = sck:recv( max )
--    msg, len  = sck:recv( adr )
--    msg, len  = sck:recv( adr,max )
--    msg, len  = sck:recv( buf )
--    msg, len  = sck:recv( buf, max )
--    msg, len  = sck:recv( adr, buf )
--    msg, len  = sck:recv( adr, buf, max )
--    msg, len  = sck:recv( [bad arguments] )
--
-- These tests run (semi-)asynchronously.  A UDP server socket is listening while each
-- test connect it's own client to it.  This allows for these tests to run within the
-- same  process.  Each test will restart the loop, connect, assert and stop the
-- loop before moving on to the next test.

local Test      = require( "t.Test" )
local Timer     = require( "t.Time" )
local Loop      = require( "t.Loop" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )
local Buffer    = require( "t.Buffer" )
local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr
local fmt       = string.format
local config    = t_require( "t_cfg" )


local makeSender = function( self, msg )
	local f = function(s, msg)
		local snt = s.cSck:send( msg, s.sAdr )
		s.loop:removeHandle( self.cSck, "write" )
	end
	self.cSck = Socket( 'udp' )
	assert( chkSck( self.cSck, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' ) )
	self.loop:addHandle( self.cSck, "write", f, self, msg )
end

local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop  = Loop( )
		self.host  = Interface.default( ).address.ip
		self.port  = config.nonPrivPort
		self.sSck  = Socket( 'udp' )
		self.sAdr  = self.sSck:bind( self.host, self.port )
		assert( chkSck( self.sSck, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' ) )
		assert( chkAdr( self.sAdr, "AF_INET", self.host, self.port ) )
		done()
	end,

	afterAll = function( self, done )
		self.sSck:close( )
		done()
	end,

	beforeEach_cb = function( self, done )
		self.loop:addTimer( Timer( 1 ), done )
		-- loop:run() blocks further execution until the function on the loop
		-- runs the afterEach_cb and releases the block forcing all tests to be
		-- executed sequentially
		self.loop:run()
	end,

	afterEach_cb = function( self, done )
		self.loop:removeHandle( self.sSck, 'read' )
		self.loop:stop( )
		self.cSck:close( )
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_recvString = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( )" )
		local payload  = string.rep( 'TestMessage content for recieving full string -- ', 14 )
		local receiver = function( s )
			local msg,len = s.sSck:recv( )
			assert( type(msg)=='string',fmt( "Expected\n%s\nbut got\n%s\b", 'string', type(msg) ) )
			assert( type(len)=='number',fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==#payload, fmt( "Expected %d but got %d bytes", #payload, len) )
			assert( msg==payload, fmt( "Expected\n%s\nbut got\n%s\b", payload, msg) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvMaxString = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( max )" )
		local payload  = string.rep( 'Receiving sized Test Message as content -- ', 12 )
		local max      = #payload//2
		local receiver = function( s )
			local msg,len = s.sSck:recv( max )
			assert( type(msg)=='string', fmt( "Expected\n%s\nbut got\n%s\b", 'string', type(msg) ) )
			assert( type(len)=='number', fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==max, fmt("Expected %d but got %d bytes", max, len ) )
			assert( msg==payload:sub( 1, max ),
			        fmt( "Expected\n%s\nbut got\n%s\b", payload:sub( 1, max ), msg ) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvStringFrom = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( adr )" )
		local payload  = string.rep( 'Receiving Test Message from Address -- ', 12 )
		local receiver = function( s )
			local adr     = Address()
			local msg,len = s.sSck:recv( adr )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(msg)=='string', fmt( "Expected\n%s\nbut got\n%s\b", 'string', type(msg) ) )
			assert( type(len)=='number', fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==#payload, fmt( "Expected %d but got %d bytes", len, #payload ) )
			assert( msg==payload, fmt( "Expected\n%s\nbut got\n%s\b", payload, msg ) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvMaxStringFrom = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( adr, max )" )
		local payload  = string.rep( 'Receiving sized Test Message from address -- ', 12 )
		local max      = #payload//2
		local receiver = function( s )
			local adr     = Address()
			local msg,len = s.sSck:recv( adr, max )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(msg)=='string', fmt( "Expected\n%s\nbut got\n%s\b", 'string', type(msg) ) )
			assert( type(len)=='number', fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==max, fmt( "Expected %d but got %d bytes", max, len ) )
			assert( msg==payload:sub( 1, max ),
			        fmt( "Expected\n%s\nbut got\n%s\b", payload:sub( 1, max ), msg ) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvBuffer = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( buf )" )
		local payload  = string.rep( 'TestMessage content for recieving full buffer -- ', 14 )
		local receiver = function( s )
			local buf     = Buffer( #payload )
			local suc,len = s.sSck:recv( buf )
			assert( type(suc)=='boolean', fmt( "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) ) )
			assert( type(len)=='number', fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==#payload, fmt( "Expected %d but got %d bytes", #payload, len) )
			assert( buf:read()==payload, fmt( "Expected\n%s\nbut got\n%s\b", payload, buf:read()) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvMaxBuffer = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( buf, max )" )
		local payload  = string.rep( 'Receiving sized Test Message into Buffer -- ', 12 )
		local max      = #payload//3
		local receiver = function( s )
			local buf     = Buffer( #payload )
			local suc,len = s.sSck:recv( buf, max )
			local a,b = buf:read(), payload:sub(1,max)
			assert( type(suc)=='boolean', fmt( "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) ) )
			assert( type(len)=='number', fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==max, fmt( "Expected %d but got %d bytes", max, len ) )
			assert( buf:read(1,max)==payload:sub( 1, max ),
			        fmt( "Expected\n_%s_\nbut got\n_%s_\b", payload:sub( 1, max ), buf:read(1,max) ) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvBufferFromAddress = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( adr, buf )" )
		local payload  = string.rep( 'TestMessage for recieving full buffer from Address-- ', 14 )
		local receiver = function( s )
			local adr,buf = Address(), Buffer( #payload )
			local suc,len = s.sSck:recv( adr, buf )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(suc)=='boolean', fmt( "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) ) )
			assert( type(len)=='number', fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==#payload, fmt( "Expected %d but got %d bytes", #payload, len) )
			assert( buf:read()==payload, fmt( "Expected\n%s\nbut got\n%s\b", payload, buf:read()) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvMaxBufferFromAddress = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( adr, buf, max )" )
		local payload  = string.rep( 'TestMessage for recieving full buffer from Address-- ', 14 )
		local max      = #payload//4
		local receiver = function( s )
			local adr,buf = Address(), Buffer( #payload )
			local suc,len = s.sSck:recv( adr, buf, max )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(suc)=='boolean', fmt( "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) ) )
			assert( type(len)=='number', fmt( "Expected\n%s\nbut got\n%s\b", 'number', type(len) ) )
			assert( len==max, fmt( "Expected %d but got %d bytes", max, len ) )
			assert( buf:read(1,max)==payload:sub( 1, max ),
			        fmt( "Expected\n_%s_\nbut got\n_%s_\b", payload:sub( 1, max ), buf:read(1,max) ) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	-- Testing the error handling of wrong arguments
	test_cb_recvStringMaxTooBigFails = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( max > BUFSIZ ) fails" )
		local payload  = string.rep( 'TestMessage for max too big', 14 )
		local eMsg     = "bad argument #1 to 'recv' %(max must be smaller than BUFSIZ%)"
		local receiver = function( s )
			local r   = function( x ) local suc,len = x.sSck:recv( 8192+3 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), fmt( "Error message should contain: `%s`\nut was\n`%s`", eMsg, e ) )
			local suc,len = s.sSck:recv( ) -- actually drain socket to allow unit test continue
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvBufferMaxTooBigFails = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( buf, max > #buf ) fails" )
		local payload  = string.rep( 'TestMessage for max too big for buffer', 14 )
		local eMsg     = "bad argument #1 to 'recv' %(max must be smaller than sink%)"
		local receiver = function( s )
			local buf = Buffer(60)
			local r   = function( x ) local suc,len = x.sSck:recv( buf, #buf+1 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), fmt( "Error message should contain: `%s`\nut was\n`%s`", eMsg, e ) )
			local suc,len = s.sSck:recv( ) -- actually drain socket to allow unit test continue
			done()
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	test_cb_recvWrongArgsFail = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( [bad arguments] ) fails" )
		local payload  = string.rep( 'TestMessage for wrong arguments', 14 )
		local len, f, eMsg, d, e  = #payload, nil, nil, nil, nil
		local receiver = function( s )
			local buf = Buffer(60)
			local adr = Buffer(60)

			-- not using sck:send but sck.send
			f    = function( x ) local msg, rcvd = x.sSck.recv( payload, "something" ) end
			eMsg = "bad argument #1 to 'recv' %(T.Net.Socket expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), fmt( "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e ) )

			f    = function( x ) local suc,len = x.sSck:recv( 'a string' ) end
			eMsg = "bad argument #1 to 'recv' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), fmt( "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e ) )

			f    = function( x ) local suc,len = x.sSck:recv( buf, 'a string' ) end
			eMsg = "bad argument #2 to 'recv' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), fmt( "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e ) )

			--[[
			r  = function( x ) local suc,len = x.sSck:recv( 20, 15 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			print( e )
			--]]
			local suc,len = s.sSck:recv( ) -- actually drain socket to allow unit test continue
			done( )
		end
		self.loop:addHandle( self.sSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

}

return Test( tests )
