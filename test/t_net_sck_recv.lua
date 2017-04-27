---
-- \file    t_net_sck_recv.lua
-- \brief   Test assuring sck:recv() works in all permutations.
-- \detail  Send and receive data via SOCK_STREAM sockets. All permutations
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
--
-- These tests run (semi-)asynchronously.  A TCP server socket is listening while each
-- test connect it's own client to it.  This allows for these tests to run within the
-- same  process.  Each test will restart the loop, connect, assert and stop the
-- loop before moving on to the next test.

Test      = require( "t.Test" )
--Test      = require( "t.tst" )
Timer     = require( "t.Time" )
Loop      = require( "t.Loop" )
Socket    = require( "t.Net.Socket" )
Address   = require( "t.Net.Address" )
Interface = require( "t.Net.Interface" )
Buffer    = require( "t.Buffer" )
Segment   = require( "t.Buffer.Segment" )
T         = require( "t" )
asrtHlp   = T.require( "assertHelper" )


local sender = function( self, msg )
	local f
	f = function(s, msg)
		local snt = s.cSck:send( msg, s.sAdr )
		s.loop:removeHandle( self.cSck, "write" )
	end
	self.loop:addHandle( self.cSck, "write", f, self, msg )
end

local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop  = Loop( 20 )
		self.host  = Interface( 'default' ).address:get( )
		self.port  = 8000
		self.sSck  = Socket( 'udp' )
		self.sAdr  = self.sSck:bind( self.host, self.port )
		asrtHlp.Socket( self.sSck, 'udp', 'AF_INET', 'SOCK_DGRAM' )
		asrtHlp.Address( self.sAdr, self.host, self.port )
		self.cSck  = Socket( 'udp' )
		asrtHlp.Socket( self.cSck, 'udp', 'AF_INET', 'SOCK_DGRAM' )
		done()
	end,

	afterAll = function( self, done )
		self.cSck:close( )
		self.sSck:close( )
		done()
	end,

	beforeEach_cb = function( self, done )
		self.loop:addTimer( Timer( 1 ), done )
		--self.loop:addHandle( self.sSck, 'read', receive, self )
		-- loop:run() blocks further execution until the function on the loop
		-- runs the afterEach_cb and releases the block forcing all tests to be
		-- executed sequentially
		self.sentBytes = 0
		self.outBuffer = nil
		self.loop:run()
	end,

	afterEach_cb = function( self, done )
		self.loop:removeHandle( self.sSck, 'read' )
		self.loop:stop( )
		--self.payload = nil
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_recvString = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( )" )
		local payload = string.rep( 'TestMessage content for recieving full string -- ', 14 )
		local recver  = function( s )
			local msg,len = s.sSck:recv( )
			T.assert( type(msg)=='string',  "Expected\n%s\nbut got\n%s\b", 'string', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==#payload, "Expected %d but got %d bytes", #payload, len)
			T.assert( msg==payload,  "Expected\n%s\nbut got\n%s\b", payload, msg)
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvMaxString = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( max )" )
		local payload = string.rep( 'Receiving sized Test Message as content -- ', 12 )
		local max     = #payload//2
		local recver  = function( s )
			local msg,len = s.sSck:recv( max )
			T.assert( type(msg)=='string',  "Expected\n%s\nbut got\n%s\b", 'string', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==max, "Expected %d but got %d bytes", max, len )
			T.assert( msg==payload:sub( 1, max ),  "Expected\n%s\nbut got\n%s\b",
			                                        payload:sub( 1, max ), msg )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvStringFrom = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( adr )" )
		local payload = string.rep( 'Receiving Test Message from Address -- ', 12 )
		local recver  = function( s )
			local adr     = Address()
			local msg,len = s.sSck:recv( adr )
			asrtHlp.Address( adr, self.host, 'any' )
			T.assert( type(msg)=='string',  "Expected\n%s\nbut got\n%s\b", 'string', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==#payload, "Expected %d but got %d bytes", len, #payload )
			T.assert( msg==payload,  "Expected\n%s\nbut got\n%s\b", payload, msg )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvMaxStringFrom = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( adr, max )" )
		local payload = string.rep( 'Receiving sized Test Message from address -- ', 12 )
		local max     = #payload//2
		local recver  = function( s )
			local adr     = Address()
			local msg,len = s.sSck:recv( adr, max )
			asrtHlp.Address( adr, self.host, 'any' )
			T.assert( type(msg)=='string',  "Expected\n%s\nbut got\n%s\b", 'string', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==max, "Expected %d but got %d bytes", max, len )
			T.assert( msg==payload:sub( 1, max ),  "Expected\n%s\nbut got\n%s\b",
			                                        payload:sub( 1, max ), msg )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvBuffer = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( buf )" )
		local payload = string.rep( 'TestMessage content for recieving full buffer -- ', 14 )
		local recver  = function( s )
			local buf     = Buffer( #payload )
			local suc,len = s.sSck:recv( buf )
			T.assert( type(suc)=='boolean',  "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==#payload, "Expected %d but got %d bytes", #payload, len)
			T.assert( buf:read()==payload,  "Expected\n%s\nbut got\n%s\b", payload, buf:read())
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvMaxBuffer = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( buf, max )" )
		local payload = string.rep( 'Receiving sized Test Message into Buffer -- ', 12 )
		local max     = #payload//3
		local recver  = function( s )
			local buf     = Buffer( #payload )
			local suc,len = s.sSck:recv( buf, max )
			local a,b = buf:read(), payload:sub(1,max)
			T.assert( type(suc)=='boolean',  "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==max, "Expected %d but got %d bytes", max, len )
			T.assert( buf:read(1,max)==payload:sub( 1, max ),  "Expected\n_%s_\nbut got\n_%s_\b",
			                                        payload:sub( 1, max ), buf:read(1,max) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvBufferFromAddress = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( adr, buf )" )
		local payload = string.rep( 'TestMessage for recieving full buffer from Address-- ', 14 )
		local recver  = function( s )
			local adr,buf = Address(), Buffer( #payload )
			local suc,len = s.sSck:recv( adr, buf )
			asrtHlp.Address( adr, self.host, 'any' )
			T.assert( type(suc)=='boolean',  "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==#payload, "Expected %d but got %d bytes", #payload, len)
			T.assert( buf:read()==payload,  "Expected\n%s\nbut got\n%s\b", payload, buf:read())
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvMaxBufferFromAddress = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( adr, buf, max )" )
		local payload = string.rep( 'TestMessage for recieving full buffer from Address-- ', 14 )
		local max     = #payload//4
		local recver  = function( s )
			local adr,buf = Address(), Buffer( #payload )
			local suc,len = s.sSck:recv( adr, buf, max )
			asrtHlp.Address( adr, self.host, 'any' )
			T.assert( type(suc)=='boolean',  "Expected\n%s\nbut got\n%s\b", 'boolean', type(msg) )
			T.assert( type(len)=='number',  "Expected\n%s\nbut got\n%s\b", 'number', type(len) )
			T.assert( len==max, "Expected %d but got %d bytes", max, len )
			T.assert( buf:read(1,max)==payload:sub( 1, max ),  "Expected\n_%s_\nbut got\n_%s_\b",
			                                        payload:sub( 1, max ), buf:read(1,max) )
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	-- Testing the error handling of wrong arguments
	test_cb_recvStringMaxTooBigFails = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( max > BUFSIZ ) fails" )
		local payload = string.rep( 'TestMessage for max too big', 14 )
		local eMsg    = "bad argument #1 to 'recv' %(max must be smaller than BUFSIZ%)"
		local recver  = function( s )
			local r   = function( x ) local suc,len = x.sSck:recv( 8192+3 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			T.assert( e:match( eMsg ), "Error message should contain: `%s`\nut was\n`%s`", eMsg, e )
			local suc,len = s.sSck:recv( ) -- actually drain socket to allow unit test continue
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvBufferMaxTooBigFails = function( self, done )
		Test.Case.describe( "suc,len = sck.recv( buf, max > #buf ) fails" )
		local payload = string.rep( 'TestMessage for max too big for buffer', 14 )
		local eMsg    = "bad argument #1 to 'recv' %(max must be smaller than sink%)"
		local recver  = function( s )
			local buf = Buffer(60)
			local r   = function( x ) local suc,len = x.sSck:recv( buf, #buf+1 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			T.assert( e:match( eMsg ), "Error message should contain: `%s`\nut was\n`%s`", eMsg, e )
			local suc,len = s.sSck:recv( ) -- actually drain socket to allow unit test continue
			done()
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	test_cb_recvWrongArgsFail = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( [bad arguments] ) fails" )
		local payload = string.rep( 'TestMessage for wrong arguments', 14 )
		local recver  = function( s )
			local buf = Buffer(60)
			local adr = Buffer(60)

			local r    = function( x ) local suc,len = x.sSck:recv( 'a string' ) end
			local eMsg = "bad argument #1 to 'recv' %(number expected, got string%)"
			local d,e  = pcall( r, s )
			assert( not d, "Call should have failed" )
			T.assert( e:match( eMsg ), "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e )
			print( e )

			r    = function( x ) local suc,len = x.sSck:recv( buf, 'a string' ) end
			eMsg = "bad argument #2 to 'recv' %(number expected, got string%)"
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			T.assert( e:match( eMsg ), "Error message should contain: `%s`\nbut was\n`%s`", eMsg, e )
			print( e )

			--[[
			r  = function( x ) local suc,len = x.sSck:recv( 20, 15 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			print( e )
			--]]
			local suc,len = s.sSck:recv( ) -- actually drain socket to allow unit test continue
			done( )
		end
		self.loop:addHandle( self.sSck, 'read', recver, self )
		sender( self, payload )
	end,

	}

t = Test( tests )
t( )
print( t )
