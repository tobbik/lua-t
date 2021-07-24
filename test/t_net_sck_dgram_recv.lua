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
local Loop      = require( "t.Loop" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )
local Buffer    = require( "t.Buffer" )
local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr
local config    = t_require( "t_cfg" )


local makeSender = function( self, msg )
	local f = function(s, msg)
		local snt = s.sndSck:send( msg, s.srvAdr )
		s.loop:removeHandle( self.sndSck, "write" )
	end
	self.sndSck = Socket( 'udp' )
	assert( chkSck( self.sndSck, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' ) )
	self.loop:addHandle( self.sndSck, "write", f, self, msg )
	self.loop:run()
end

return {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self )
		self.loop  = Loop( )
		self.host  = Interface.default( ).address.ip
		self.port  = config.nonPrivPort
		self.srvSck  = Socket( 'udp' )
		self.srvAdr  = self.srvSck:bind( self.host, self.port )
		assert( chkSck( self.srvSck, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' ) )
		assert( chkAdr( self.srvAdr, "AF_INET", self.host, self.port ) )
	end,

	afterAll = function( self )
		self.srvSck:close( )
	end,

	afterEach = function( self )
		self.sndSck:close( )
	end,

	-- #########################################################################
	-- Actual Test cases
	recvString = function( self )
		Test.describe( "msg,len = sck.recv( )" )
		local payload  = string.rep( 'TestMessage content for recieving full string -- ', 14 )
		local receiver = function( s )
			local msg,len = s.srvSck:recv( )
			assert( type(msg)=='string',("Expected\n%s\nbut got\n%s\b"):format( 'string', type(msg) ) )
			assert( type(len)=='number',("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==#payload, ("Expected %d but got %d bytes"):format( #payload, len) )
			assert( msg==payload, ("Expected\n%s\nbut got\n%s\b"):format( payload, msg) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvMaxString = function( self )
		Test.describe( "msg,len = sck.recv( max )" )
		local payload  = string.rep( 'Receiving sized Test Message as content -- ', 12 )
		local max      = #payload//2
		local receiver = function( s )
			local msg,len = s.srvSck:recv( max )
			assert( type(msg)=='string', ("Expected\n%s\nbut got\n%s\b"):format( 'string', type(msg) ) )
			assert( type(len)=='number', ("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==max, ("Expected %d but got %d bytes"):format( max, len ) )
			assert( msg==payload:sub( 1, max ),
			        ("Expected\n%s\nbut got\n%s\b"):format( payload:sub( 1, max ), msg ) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvStringFrom = function( self )
		Test.describe( "msg,len = sck.recv( adr )" )
		local payload  = string.rep( 'Receiving Test Message from Address -- ', 12 )
		local receiver = function( s )
			local adr     = Address()
			local msg,len = s.srvSck:recv( adr )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(msg)=='string', ("Expected\n%s\nbut got\n%s\b"):format( 'string', type(msg) ) )
			assert( type(len)=='number', ("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==#payload, ("Expected %d but got %d bytes"):format( len, #payload ) )
			assert( msg==payload, ("Expected\n%s\nbut got\n%s\b"):format( payload, msg ) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvMaxStringFrom = function( self )
		Test.describe( "msg,len = sck.recv( adr, max )" )
		local payload  = string.rep( 'Receiving sized Test Message from address -- ', 12 )
		local max      = #payload//2
		local receiver = function( s )
			local adr     = Address()
			local msg,len = s.srvSck:recv( adr, max )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(msg)=='string', ("Expected\n%s\nbut got\n%s\b"):format( 'string', type(msg) ) )
			assert( type(len)=='number', ("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==max, ("Expected %d but got %d bytes"):format( max, len ) )
			assert( msg==payload:sub( 1, max ),
			        ("Expected\n%s\nbut got\n%s\b"):format( payload:sub( 1, max ), msg ) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvBuffer = function( self )
		Test.describe( "suc,len = sck.recv( buf )" )
		local payload  = string.rep( 'TestMessage content for recieving full buffer -- ', 14 )
		local receiver = function( s )
			local buf     = Buffer( #payload )
			local suc,len = s.srvSck:recv( buf )
			assert( type(suc)=='boolean', ("Expected\n%s\nbut got\n%s\b"):format( 'boolean', type(msg) ) )
			assert( type(len)=='number', ("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==#payload, ("Expected %d but got %d bytes"):format( #payload, len) )
			assert( buf:read()==payload, ("Expected\n%s\nbut got\n%s\b"):format( payload, buf:read()) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvMaxBuffer = function( self )
		Test.describe( "suc,len = sck.recv( buf, max )" )
		local payload  = string.rep( 'Receiving sized Test Message into Buffer -- ', 12 )
		local max      = #payload//3
		local receiver = function( s )
			local buf     = Buffer( #payload )
			local suc,len = s.srvSck:recv( buf, max )
			local a,b = buf:read(), payload:sub(1,max)
			assert( type(suc)=='boolean', ("Expected\n%s\nbut got\n%s\b"):format( 'boolean', type(msg) ) )
			assert( type(len)=='number', ("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==max, ("Expected %d but got %d bytes"):format( max, len ) )
			assert( buf:read(1,max)==payload:sub(1,max ),
			        ("Expected\n_%s_\nbut got\n_%s_\b"):format( payload:sub( 1, max ), buf:read(1,max) ) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvBufferFromAddress = function( self )
		Test.describe( "suc,len = sck.recv( adr, buf )" )
		local payload  = string.rep( 'TestMessage for recieving full buffer from Address-- ', 14 )
		local receiver = function( s )
			local adr,buf = Address(), Buffer( #payload )
			local suc,len = s.srvSck:recv( adr, buf )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(suc)=='boolean', ("Expected\n%s\nbut got\n%s\b"):format( 'boolean', type(msg) ) )
			assert( type(len)=='number', ("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==#payload, ("Expected %d but got %d bytes"):format( #payload, len) )
			assert( buf:read()==payload, ("Expected\n%s\nbut got\n%s\b"):format( payload, buf:read()) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvMaxBufferFromAddress = function( self )
		Test.describe( "suc,len = sck.recv( adr, buf, max )" )
		local payload  = string.rep( 'TestMessage for recieving full buffer from Address-- ', 14 )
		local max      = #payload//4
		local receiver = function( s )
			local adr,buf = Address(), Buffer( #payload )
			local suc,len = s.srvSck:recv( adr, buf, max )
			assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
			assert( type(suc)=='boolean', ("Expected\n%s\nbut got\n%s\b"):format( 'boolean', type(msg) ) )
			assert( type(len)=='number', ("Expected\n%s\nbut got\n%s\b"):format( 'number', type(len) ) )
			assert( len==max, ("Expected %d but got %d bytes"):format( max, len ) )
			assert( buf:read(1,max)==payload:sub( 1, max ),
			        ("Expected\n_%s_\nbut got\n_%s_\b"):format( payload:sub( 1, max ), buf:read(1,max) ) )
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	-- Testing the error handling of wrong arguments
	recvStringMaxTooBigFails = function( self )
		Test.describe( "suc,len = sck.recv( max > BUFSIZ ) fails" )
		local payload  = string.rep( 'TestMessage for max too big', 14 )
		local eMsg     = "bad argument #1 to 'recv' %(max must be smaller than BUFSIZ%)"
		local receiver = function( s )
			local r   = function( x ) local suc,len = x.srvSck:recv( 8192+3 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nut was\n`%s`"):format( eMsg, e ) )
			local suc,len = s.srvSck:recv( ) -- actually drain socket to allow unit test continue
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvBufferMaxTooBigFails = function( self )
		Test.describe( "suc,len = sck.recv( buf, max > #buf ) fails" )
		local payload  = string.rep( 'TestMessage for max too big for buffer', 14 )
		local eMsg     = "bad argument #1 to 'recv' %(max must be smaller than sink%)"
		local receiver = function( s )
			local buf = Buffer(60)
			local r   = function( x ) local suc,len = x.srvSck:recv( buf, #buf+1 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nut was\n`%s`"):format( eMsg, e ) )
			local suc,len = s.srvSck:recv( ) -- actually drain socket to allow unit test continue
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

	recvWrongArgsFail = function( self )
		Test.describe( "msg,len = sck.recv( [bad arguments] ) fails" )
		local payload  = string.rep( 'TestMessage for wrong arguments', 14 )
		local len, f, eMsg, d, e  = #payload, nil, nil, nil, nil
		local receiver = function( s )
			local buf = Buffer(60)
			local adr = Buffer(60)

			-- not using sck:send but sck.send
			f    = function( x ) local msg, rcvd = x.srvSck.recv( payload, "something" ) end
			eMsg = "bad argument #1 to 'recv' %(T.Net.Socket expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nbut was\n`%s`"):format( eMsg, e ) )

			f    = function( x ) local suc,len = x.srvSck:recv( 'a string' ) end
			eMsg = "bad argument #1 to 'recv' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nbut was\n`%s`"):format( eMsg, e ) )

			f    = function( x ) local suc,len = x.srvSck:recv( buf, 'a string' ) end
			eMsg = "bad argument #2 to 'recv' %(number expected, got string%)"
			d,e  = pcall( f, s )
			assert( not d, "Call should have failed" )
			assert( e:match( eMsg ), ("Error message should contain: `%s`\nbut was\n`%s`"):format( eMsg, e ) )

			--[[
			r  = function( x ) local suc,len = x.srvSck:recv( 20, 15 ) end
			local d,e = pcall( r, s )
			assert( not d, "Call should have failed" )
			print( e )
			--]]
			local suc,len = s.srvSck:recv( ) -- actually drain socket to allow unit test continue
			s.loop:removeHandle( s.srvSck, 'read' )
		end
		self.loop:addHandle( self.srvSck, 'read', receiver, self )
		makeSender( self, payload )
	end,

}
