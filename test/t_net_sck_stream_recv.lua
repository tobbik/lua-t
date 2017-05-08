---
-- \file    test/t_net_sck_stream_recv.lua
-- \brief   Test assuring sck:recv() works in all permutations.
-- \detail  Receive data via SOCK_STREAM sockets. All permutations
--          tested in this suite:
--
--    msg, len  = sck:recv( )
--    msg, len  = sck:recv( max )
--    msg, len  = sck:recv( buf )
--    msg, len  = sck:recv( buf, max )
--    msg, len  = sck:recv( [bad arguments] )
--
-- These tests run (semi-)asynchronously.  A TCP server socket is listening
-- while each test connect it's own client to it.  This allows for these tests
-- to run within the same  process.  Each test will restart the loop, connect,
-- assert and stop the loop before moving on to the next test.

Test      = require( "t.Test" )
Timer     = require( "t.Time" )
Loop      = require( "t.Loop" )
Socket    = require( "t.Net.Socket" )
Address   = require( "t.Net.Address" )
Interface = require( "t.Net.Interface" )
Buffer    = require( "t.Buffer" )
Segment   = require( "t.Buffer.Segment" )
T         = require( "t" )
asrtHlp   = T.require( "assertHelper" )


-- for this test we use blocking sending only.  The test is for recv(); no need
-- to complicate it
local makeSender = function( self, msg )
	self.cSck = Socket.connect( self.sAdr )
	local f = function( s, m )
		local snt = s.cSck:send( m )
		T.assert( snt == #m, "Should have sent all(%d bytes) but sent %d bytes", #m, snt )
		s.loop:removeHandle( self.cSck, "write" )
		self.cSck:shutdown( 'write' )
	end
	self.loop:addHandle( self.cSck, 'write', f, self, msg )
end

-- #########################################################################
-- accept server for each test and set up recv()
local makeReceiver = function( self, receiver )
	local acpt = function( )
		self.aSck, self.aAdr = self.sSck:accept( )
		asrtHlp.Socket( self.aSck, "tcp", "AF_INET", "SOCK_STREAM" )
		asrtHlp.Address( self.aAdr, self.host, "any" )
		self.loop:addHandle( self.aSck, "read", receiver, self )
	end
	self.loop:addHandle( self.sSck, "read", acpt )
end


local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop            = Loop( 20 )
		self.host            = Interface( 'default' ).address:get( )
		self.port            = 8000
		self.sSck, self.sAdr = Socket.listen( self.host, self.port )
		asrtHlp.Socket( self.sSck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.sAdr, self.host, self.port )
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
		self.loop:removeHandle( self.aSck, 'read' )
		self.cSck:close( )
		self.aSck:close( )
		self.loop:stop( )
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_recvString = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( )" )
		local payload  = string.rep( "TestMessage content for recieving full string -- ", 60000 )
		local cnt      = 0
		local receiver = function( s )
			local msg,len = s.aSck:recv( )
			T.assert( type(len)=='number',  "Expected `%s` but got `%s`", 'number', type(len) )
			if msg then
				T.assert( type(msg)=='string',  "Expected `%s` but got `%s`", 'string', type(msg) )
				T.assert( msg == payload:sub(cnt+1,cnt+len), "Message was %s but should have been %s",
				          msg, payload:sub(cnt+1,cnt+len) )
				cnt = cnt+len
			else
				T.assert( cnt==#payload, "Expected %d but got %d bytes", #payload, cnt)
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvSizedString = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( size )" )
		local payload  = string.rep( "TestMessage content for recieving chopped up chunky stringes -- ", 60000 )
		-- #payload must be devisible by sz - else test fails on last chunk!
		local rcvd,sz,cnt  = 0,128,0
		local receiver = function( s )
			local msg,len = s.aSck:recv( sz )
			T.assert( type(len)=='number',  "Expected `%s` but got `%s`", 'number', type(len) )
			if msg then
				T.assert( sz==len,  "Expected %d bytes but got %d", sz, len )
				T.assert( type(msg)=='string',  "Expected `%s` but got `%s`", 'string', type(msg) )
				T.assert( msg == payload:sub(rcvd+1,rcvd+len), "Message was %s but should have been %s",
				          msg, payload:sub(rcvd+1,rcvd+len) )
				rcvd = rcvd+len
				cnt  = cnt +1
			else
				T.assert( rcvd==#payload, "Expected %d but got %d bytes", #payload, rcvd)
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvBuffer = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( buf_seg )" )
		local payload  = string.rep( "TestMessage content for recieving bigger buffer -- ", 90000 )
		local buffer   = Buffer( #payload )
		local cnt      = 0
		local receiver = function( s )
			local msg,len = s.aSck:recv( Segment( buffer, cnt+1 ) )
			T.assert( type(len)=='number',  "Expected `%s` but got `%s`", 'number', type(len) )
			if msg then
				T.assert( type(msg)=='boolean',  "Expected `%s` but got `%s`", 'boolean', type(msg) )
				cnt = cnt+len
			else
				T.assert( cnt==#payload, "Expected %d but got %d bytes", #payload, cnt)
				T.assert( buffer:read()==payload, "Payload should equal received value")
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvSizedBuffer = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( buf_seg )  [Small 128 bytes segment]" )
		local payload  = string.rep( "TestMessage content for recieving bigger buffer -- ", 90000 )
		local buffer   = Buffer( #payload )
		local rcvd, sz, cnt  = 0,128, 0
		local receiver = function( s )
			local seg     = cnt+1+sz > #buffer and Segment( buffer, cnt+1 )
			                                   or  Segment( buffer, cnt+1, 128 )
			local msg,len = s.aSck:recv( seg )
			T.assert( type(len)=='number',  "Expected `%s` but got `%s`", 'number', type(len) )
			if msg then
				T.assert( #seg==len,  "Expected %d bytes but got %d", #seg, len )
				T.assert( type(msg)=='boolean',  "Expected `%s` but got `%s`", 'boolean', type(msg) )
				cnt = cnt+len
			else
				T.assert( cnt==#payload, "Expected %d but got %d bytes", #payload, cnt)
				T.assert( buffer:read()==payload, "Payload should equal received value")
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvSizedBufferMax = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( buffer, size )" )
		local payload  = string.rep( "TestMessage content for recieving chopped up chunky stringes -- ", 60000 )
		-- #payload must be devisible by sz - else test fails on last chunk!
		local rcvd,sz,cnt  = 0,128,0
		local receiver = function( s )
			local buf     = Buffer( 2*sz )
			local msg,len = s.aSck:recv( buf, sz )
			T.assert( type(len)=='number',  "Expected `%s` but got `%s`", 'number', type(len) )
			if msg then
				T.assert( sz==len,  "Expected %d bytes but got %d", sz, len )
				T.assert( buf:read(1,len) == payload:sub(rcvd+1,rcvd+len), "Message was %s but should have been %s",
				          buf:read(1,len), payload:sub(rcvd+1,rcvd+len) )
				rcvd = rcvd+len
				cnt  = cnt +1
			else
				T.assert( rcvd==#payload, "Expected %d but got %d bytes", #payload, rcvd)
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	}

return Test( tests )
