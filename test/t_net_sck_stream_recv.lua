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
local Loop      = require( "t.Loop" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )
local Buffer    = require( "t.Buffer" )

local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr
local config    = t_require( "t_cfg" )

local snd_buffer_max = 1048576 -- according to manpage


-- #########################################################################
-- accept server for each test and set up recv()
local makeReceiver = function( self, receiver )
	local acpt = function( n )
		n.rcvSck, n.rcvAdr = n.srvSck:accept( )
		n.loop:addHandle( n.rcvSck, "read", receiver, n )
		n.loop:removeHandle( n.srvSck, "read" )
	end
	self.loop:addHandle( self.srvSck, "read", acpt, self )
end

local prepareSender = function( self, msg )
	self.sndSck      = Socket( )
	self.sndSck.reuseaddr, self.sndSck.reuseport = true,true
	local bndAdr,_   = self.sndSck:bind( self.host, self.portAlt )
	--print("BOUND", bndAdr, _ )
	self.sndSck:connect( self.srvAdr )
	if self.sndSck.sendbuffer < snd_buffer_max then
		self.sndSck.sendbuffer = snd_buffer_max
	end
	return msg:rep( self.sndSck.sendbuffer // #msg - 1000 )
end

-- for this test we use blocking sending only.  The test is for recv(); no need
-- to complicate it
local makeSender = function( self, payload )
	assert( self.sndSck.sendbuffer >= #payload,
		("Sendbuffer[%d] is smaller than #payload[%d]. Decrease payload size."):format( self.sndSck.sendbuffer, #payload ) )
	local f = function( n, p )
		local snt = n.sndSck:send( p )
		assert( snt == #p, ("Should have sent all(%d bytes) but sent %d bytes"):format( #p, snt ) )
		n.loop:removeHandle( n.sndSck, "write" )
		--n.sndSck:shutdown( "wr" )
		n.sndSck:close( )
	end
	self.loop:addHandle( self.sndSck, 'write', f, self, payload )
	self.loop:run( )
end

return {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self )
		self.loop                  = Loop( )
		self.host                  = Interface.default( ).address.ip
		self.port, self.portAlt    = 1500, 1501
		self.srvSck                = Socket()
		self.srvSck.reuseaddr, self.srvSck.reuseport = true,true
		self.srvAdr  = self.srvSck:listen( self.host, self.port )
		assert( chkSck( self.srvSck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.srvAdr, "AF_INET", self.host, self.port ) )
	end,

	afterAll = function( self )
		self.srvSck:close( )
	end,

	--afterEach = function( self )
	--	self.rcvSck:close( )
	--	self.sndSck:close( )
	--end,

	-- #########################################################################
	-- Actual Test cases
	recvString = function( self )
		Test.describe( "msg,len = sck.recv( )" )
		local payload  = prepareSender( self, "TestMessage content for recieving full string -- " )
		local cnt      = 0
		local receiver = function( s )
			local msg,len = s.rcvSck:recv( )
			assert( type(len)=='number', ("Expected `%s` but got `%s`"):format( 'number', type(len) ) )
			if msg then
				assert( type(msg)=='string', ("Expected `%s` but got `%s`"):format( 'string', type(msg) ) )
				assert( msg == payload:sub(cnt+1,cnt+len), ("Message was %s but should have been %s"):format(
				          msg, payload:sub(cnt+1,cnt+len) ) )
				cnt = cnt+len
			else
				assert( cnt==#payload, ("Expected %d but got %d bytes"):format( #payload, cnt) )
				s.loop:clean()
				s.rcvSck:close( )
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	recvSizedString = function( self )
		Test.describe( "msg,len = sck.recv( size )" )
		local payload  = prepareSender( self, "TestMessage content for recieving chopped up chunky stringes -- " )
		-- #payload must be devisible by sz - else test fails on last chunk!
		local rcvd,sz,cnt  = 0,128,0
		local receiver = function( s )
			local msg,len = s.rcvSck:recv( sz )
			assert( type(len)=='number', ("Expected `%s` but got `%s`"):format( 'number', type(len) ) )
			if msg then
				assert( len <= sz, ("Expected %d bytes but got %d"):format( sz, len ) )
				assert( type(msg)=='string', ("Expected `%s` but got `%s`"):format( 'string', type(msg) ) )
				assert( msg == payload:sub(rcvd+1,rcvd+len), ("Message was %s but should have been %s"):format(
				          msg, payload:sub(rcvd+1,rcvd+len) ) )
				rcvd = rcvd+len
				cnt  = cnt +1
			else
				assert( rcvd==#payload, ("Expected %d but got %d bytes"):format( #payload, rcvd) )
				s.loop:clean()
				s.rcvSck:close( )
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	recvBuffer = function( self )
		Test.describe( "msg,len = sck.recv( buf_seg )" )
		local payload  = prepareSender( self, "TestMessage content for recieving bigger buffer -- " )
		local buffer   = Buffer( #payload ) -- empty buffer size of payload
		local seg      = buffer:Segment()   -- cover entire Buffer
		local cnt      = 0
		local receiver = function( s )
			local msg,len = s.rcvSck:recv( seg )
			assert( type(len)=='number', ("Expected `%s` but got `%s`"):format( 'number', type(len) ) )
			if msg then
				assert( type(msg)=='boolean', ("Expected `%s` but got `%s`"):format( 'boolean', type(msg) ) )
				cnt = cnt+len
				if cnt < #buffer then
					seg.start = cnt+1
				end
			else
				assert( cnt==#payload, ("Expected %d but got %d bytes"):format( #payload, cnt) )
				assert( buffer:read()==payload, "Payload should equal received value" )
				s.loop:clean()
				s.rcvSck:close( )
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	recvSizedBuffer = function( self )
		Test.describe( "msg,len = sck.recv( buf_seg )  [Small 128 bytes segment]" )
		local payload  = prepareSender( self, "TestMessage content for recieving into a small sized buffer --- " )
		local buffer   = Buffer( #payload )
		local seg, cnt = buffer:Segment( 1, 128 ), 0
		local receiver = function( s )
			local msg,len = s.rcvSck:recv( seg )
			assert( type(len)=='number', ("Expected `%s` but got `%s`"):format( 'number', type(len) ) )
			if msg then
				assert( len <= #seg, ("Expected %d bytes but got %d"):format( #seg, len ) )
				assert( type(msg)=='boolean', ("Expected `%s` but got `%s`"):format( 'boolean', type(msg) ) )
				cnt = cnt+len
				if len==#seg then
					seg:next()
				else
					seg:shift( len )
				end
			else
				assert( cnt==#payload, ("Expected %d but got %d bytes"):format( #payload, cnt) )
				assert( buffer:read()==payload, "Payload should equal received value" )
				s.loop:clean()
				s.rcvSck:close()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,

	recvSizedBufferMax = function( self )
		Test.describe( "msg,len = sck.recv( buffer, size )" )
		local payload  = prepareSender( self, "TestMessage content for recieving chopped up chunky stringes -- " )
		-- #payload must be devisible by sz - else test fails on last chunk!
		local rcvd,sz,cnt  = 0,128,0
		local receiver = function( s )
			local buf     = Buffer( 2*sz )
			local msg,len = s.rcvSck:recv( buf, sz )
			assert( type(len)=='number', ("Expected `%s` but got `%s`"):format( 'number', type(len) ) )
			if msg then
				assert( len <= sz, ("Expected %d bytes but got %d"):format( sz, len ) )
				assert( buf:read(1,len) == payload:sub(rcvd+1,rcvd+len), ("Message was %s but should have been %s"):format(
				          buf:read(1,len), payload:sub(rcvd+1,rcvd+len) ) )
				rcvd = rcvd+len
				cnt  = cnt +1
			else
				assert( rcvd==#payload, ("Expected %d but got %d bytes"):format( #payload, rcvd) )
				s.loop:clean()
				s.rcvSck:close()
			end
		end
		makeReceiver( self, receiver )
		makeSender( self, payload )
	end,
}
