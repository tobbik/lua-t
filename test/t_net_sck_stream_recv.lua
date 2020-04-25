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


-- for this test we use blocking sending only.  The test is for recv(); no need
-- to complicate it
local makeSender = function( self, msg )
	self.cSck = Socket.connect( self.sAdr )
	assert( self.cSck.sendbuffer >= #msg,
		fmt( "Sendbuffer[%d] is smaller than #msg[%d]. Consider increasing payload size.", self.cSck.sendbuffer, #msg ) )
	local f = function( s, m )
		local snt = s.cSck:send( m )
		assert( snt == #m, fmt( "Should have sent all(%d bytes) but sent %d bytes", #m, snt ) )
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
		assert( chkSck( self.aSck, "IPPROTO_TCP", "AF_INET", "SOCK_STREAM" ) )
		assert( chkAdr( self.aAdr, "AF_INET", self.host, "any" ) )
		self.loop:addHandle( self.aSck, "read", receiver, self )
	end
	self.loop:addHandle( self.sSck, "read", acpt )
end


local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop            = Loop( )
		self.host            = Interface.default( ).AF_INET.address.ip
		self.port            = config.nonPrivPort
		self.sSck, self.sAdr = Socket.listen( self.host, self.port )
		assert( chkSck( self.sSck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
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
		local payload  = string.rep( "TestMessage content for recieving full string -- ", 50000 )
		local cnt      = 0
		local receiver = function( s )
			local msg,len = s.aSck:recv( )
			assert( type(len)=='number', fmt( "Expected `%s` but got `%s`", 'number', type(len) ) )
			if msg then
				assert( type(msg)=='string', fmt( "Expected `%s` but got `%s`", 'string', type(msg) ) )
				assert( msg == payload:sub(cnt+1,cnt+len), fmt( "Message was %s but should have been %s",
				          msg, payload:sub(cnt+1,cnt+len) ) )
				cnt = cnt+len
			else
				assert( cnt==#payload, fmt( "Expected %d but got %d bytes", #payload, cnt) )
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvSizedString = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( size )" )
		local payload  = string.rep( "TestMessage content for recieving chopped up chunky stringes -- ", 35000 )
		-- #payload must be devisible by sz - else test fails on last chunk!
		local rcvd,sz,cnt  = 0,128,0
		local receiver = function( s )
			local msg,len = s.aSck:recv( sz )
			assert( type(len)=='number', fmt( "Expected `%s` but got `%s`", 'number', type(len) ) )
			if msg then
				assert( len <= sz, fmt( "Expected %d bytes but got %d", sz, len ) )
				assert( type(msg)=='string', fmt( "Expected `%s` but got `%s`", 'string', type(msg) ) )
				assert( msg == payload:sub(rcvd+1,rcvd+len), fmt( "Message was %s but should have been %s",
				          msg, payload:sub(rcvd+1,rcvd+len) ) )
				rcvd = rcvd+len
				cnt  = cnt +1
			else
				assert( rcvd==#payload, fmt( "Expected %d but got %d bytes", #payload, rcvd) )
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvBuffer = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( buf_seg )" )
		local payload  = string.rep( "TestMessage content for recieving bigger buffer -- ", 50000 )
		local buffer   = Buffer( #payload ) -- empty buffer size of payload
		local seg      = buffer:Segment()   -- cover entire Buffer
		local cnt      = 0
		local receiver = function( s )
			local msg,len = s.aSck:recv( seg )
			assert( type(len)=='number', fmt( "Expected `%s` but got `%s`", 'number', type(len) ) )
			if msg then
				assert( type(msg)=='boolean', fmt( "Expected `%s` but got `%s`", 'boolean', type(msg) ) )
				cnt = cnt+len
				if cnt < #buffer then
					seg.start = cnt+1
				end
			else
				assert( cnt==#payload, fmt( "Expected %d but got %d bytes", #payload, cnt) )
				assert( buffer:read()==payload, fmt( "Payload should equal received value") )
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvSizedBuffer = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( buf_seg )  [Small 128 bytes segment]" )
		local payload  = string.rep( "TestMessage content for recieving into a small sized buffer --- ", 35000 )
		local buffer   = Buffer( #payload )
		local seg, cnt = buffer:Segment( 1, 128 ), 0
		local receiver = function( s )
			local msg,len = s.aSck:recv( seg )
			assert( type(len)=='number', fmt( "Expected `%s` but got `%s`", 'number', type(len) ) )
			if msg then
				assert( len <= #seg, fmt( "Expected %d bytes but got %d", #seg, len ) )
				assert( type(msg)=='boolean', fmt( "Expected `%s` but got `%s`", 'boolean', type(msg) ) )
				cnt = cnt+len
				if len==#seg then
					seg:next()
				else
					seg:shift( len )
				end
			else
				assert( cnt==#payload, fmt( "Expected %d but got %d bytes", #payload, cnt) )
				assert( buffer:read()==payload, fmt( "Payload should equal received value") )
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	test_cb_recvSizedBufferMax = function( self, done )
		Test.Case.describe( "msg,len = sck.recv( buffer, size )" )
		local payload  = string.rep( "TestMessage content for recieving chopped up chunky stringes -- ", 40000 )
		-- #payload must be devisible by sz - else test fails on last chunk!
		local rcvd,sz,cnt  = 0,128,0
		local receiver = function( s )
			local buf     = Buffer( 2*sz )
			local msg,len = s.aSck:recv( buf, sz )
			assert( type(len)=='number', fmt( "Expected `%s` but got `%s`", 'number', type(len) ) )
			if msg then
				assert( len <= sz, fmt( "Expected %d bytes but got %d", sz, len ) )
				assert( buf:read(1,len) == payload:sub(rcvd+1,rcvd+len), fmt( "Message was %s but should have been %s",
				          buf:read(1,len), payload:sub(rcvd+1,rcvd+len) ) )
				rcvd = rcvd+len
				cnt  = cnt +1
			else
				assert( rcvd==#payload, fmt( "Expected %d but got %d bytes", #payload, rcvd) )
				done()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

}

return Test( tests )
