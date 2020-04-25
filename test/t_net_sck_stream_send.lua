#!../out/bin/lua

---
-- \file    test/t_net_sck_stream_send.lua
-- \brief   Test assuring s.send(...) working on SOCK_STREAM sockets
-- \detail  Send and receive data via SOCK_STREAM sockets. All permutations
--          tested in this suite:
--                   s:snd( str )
--                   s:snd( buf )
--                   s:snd( buf_seg )
--                   s:snd( str, size )
--                   s:snd( buf, size )
--                   s:snd( buf_seg, size )
-- In reality, sending to a Net.Address via a "SOCK_STREAM" type socket is not
-- really a reasonable application and hence is not covered in unit tests.
-- These tests run (semi-)asynchronously.  A TCP server socket is listening
-- while eachtest connect it's own client to it.  This allows for these tests
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

local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr
local fmt       = string.format
local config    = t_require( "t_cfg" )


-- #########################################################################
-- accept server for each test and set up recv()
local makeReceiver = function( self, size, payload, done )
	local inCount, incBuffer, sck, adr = 0, Buffer( size ), nil, nil
	local rcv = function( )
		local seg     = inCount < #incBuffer and incBuffer:Segment( inCount+1 ) or incBuffer:Segment( #incBuffer,0 )
		local suc,cnt = sck:recv( seg )
		if suc then
			assert( incBuffer:read( inCount+1, cnt ) == payload:sub( inCount+1, cnt+inCount ),
				fmt( "%d[%d]Received and original should match:\n%s\n---------------------------\n%s",
				inCount, cnt, incBuffer:read( inCount+1, cnt ), payload:sub( inCount+1, cnt+inCount  ) ) )
			inCount = cnt + inCount
		else
			assert( inCount  ==  size, fmt( "Send(%d) and Recv(%d) count should be equal", size, inCount ) )
			assert( incBuffer:read() == payload, fmt( "Sent payload shouls equal received overall message" ) )
			self.loop:removeHandle( sck, "read" )
			self.cSck:close( )
			sck:close( )
			done( )
		end
	end
	local acpt = function( )
		sck, adr = self.sSck:accept( )
		assert( chkSck( sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( adr, "AF_INET", self.host, 'any' ) )
		self.loop:addHandle( sck, "read", rcv )
	end
	self.loop:addHandle( self.sSck, "read", acpt )
end

local makeSender = function( self, sender, nonblock )
	self.cSck:connect( self.sAdr )
	self.loop:addHandle( self.cSck, 'write', sender, self )
	if nonblock then self.cSck.nonblock = true end
end

local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop            = Loop( )
		self.host            = Interface.default( ).AF_INET.address.ip
		self.port            = config.nonPrivPort
		self.sSck, self.sAdr = Socket.listen( self.host, self.port )
		done( )
	end,

	afterAll = function( self, done )
		self.sSck:close( )
		done( self )
	end,

	beforeEach_cb = function( self, done )
		self.loop:addTimer( Timer( 1 ), done )
		self.cSck = Socket( "TCP" )
		-- loop:run() blocks further execution until the function on the loop
		-- runs the afterEach_cb and releases the block forcing all tests to be
		-- executed sequentially
		self.loop:run()
	end,

	afterEach_cb = function( self, done )
		self.loop:removeHandle( self.sSck, 'read' )
		self.loop:stop( )
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_sendFullString = function( self, done )
		Test.Case.describe( "cnt = sck.send( msg ) -- send all in one go" )
		local payload = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 41000 )
		local sender  = function( s )
			assert( s.cSck.sendbuffer >= #payload,
			        fmt( "Sendbuffer[%d] is smaller than #msg[%d]. Consider adjusting size.", s.cSck.sendbuffer, #payload ) )
			local cnt = s.cSck:send( payload )
			assert( cnt == #payload, fmt( "Blocking send() should send all(%d) but sent(%d)", #payload, cnt ) )
			s.loop:removeHandle( s.cSck, 'write' )
			s.cSck:shutdown( 'write' )
		end
		makeReceiver( self, #payload, payload, done )
		makeSender( self, sender )
	end,

	test_cb_sendStringSmallChunks = function( self, done )
		Test.Case.describe( "cnt = sck.send( msg, sz ) -- small sized chunks" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 40000 )
		local chunk_sz = 128
		local sender = function( s )
			-- the substring only has to be longer than chunksize
			local cnt = s.cSck:send( payload:sub( outCount+1, outCount+chunk_sz ), chunk_sz )
			if cnt>0 then
				if cnt < chunk_sz then
					-- check for proper remainder size
					assert( cnt == #payload % chunk_sz, fmt('Sent chunk was #%d, expected %d', cnt, #payload % chunk_sz ) )
				else
					assert( cnt == chunk_sz, fmt('Sent chunk was #%d, expected %d', cnt, chunk_sz ) )
				end
				sendCount = sendCount+1
				outCount  = cnt + outCount
			else
				assert( sendCount == math.ceil( outCount/chunk_sz ),
				        fmt( "expected iterations: %d/%d", sendCount, math.ceil( outCount/chunk_sz ) ) )
				assert( outCount  == #payload, fmt( "send() should accumulate to (%d) but sent(%d)", #payload, outCount ) )
				s.loop:removeHandle( s.cSck, 'write' )
				s.cSck:shutdown( 'write' )
			end
		end
		makeReceiver( self, #payload, payload, done )
		makeSender( self, sender )
	end,

	test_cb_sendStringNonBlocking = function( self, done )
		Test.Case.describe( "cnt = sck.send( msg ) on nonblocking socket" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 600000 )
		local sender = function( s )
			local cnt = s.cSck:send( payload:sub( outCount+1 ) )
			if cnt > 0 then
				sendCount = sendCount+1
				outCount  = cnt + outCount
			else
				assert( sendCount>1, fmt( "Non blocking should have broken up sending: %d ", sendCount) )
				assert( outCount  == #payload, fmt( "send() should accumulate to (%d) but sent(%d)", #payload, outCount ) )
				s.loop:removeHandle( s.cSck, 'write' )
				s.cSck:shutdown( 'write' )
			end
		end
		makeReceiver( self, #payload, payload, done )
		makeSender( self, sender, true )
	end,

	test_cb_sendFullBuffer = function( self, done )
		Test.Case.describe( "cnt = sck.send( buf ) -- send all in one go" )
		local payload  = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 41000 )
		local buf      = Buffer( payload )
		local sender = function( s )
			assert( s.cSck.sendbuffer >= #payload,
			        fmt( "Sendbuffer[%d] is smaller than #msg[%d]. Consider adjusting size.", s.cSck.sendbuffer, #payload ) )
			local cnt = s.cSck:send( buf )
			assert( cnt == #buf, fmt( "Blocking send() should send all(%d) but sent(%d)", #buf, cnt ) )
			s.loop:removeHandle( s.cSck, 'write' )
			s.cSck:shutdown( 'write' )
		end
		makeReceiver( self, #payload, payload, done )
		makeSender( self, sender )
	end,

	test_cb_sendBufferSmallChunks = function( self, done )
		Test.Case.describe( "cnt = sck.send( buf_seg ) -- small sized chunks" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 50000 )
		local buf      = Buffer( payload )
		local chunk_sz = 128
		local seg      = buf:Segment( 1, chunk_sz )
		local sender   = function( s )
			local cnt = s.cSck:send( seg )
			outCount  = outCount + cnt
			sendCount = sendCount+1
			if outCount == #buf then
				assert( sendCount == math.ceil( outCount/chunk_sz ),
				        fmt( "expected iterations: %d/%d", sendCount, math.ceil( outCount/chunk_sz ) ) )
				assert( outCount  == #buf, fmt( "send() should accumulate to (%d) but sent(%d)", #buf, outCount ) )
				s.loop:removeHandle( s.cSck, 'write' )
				s.cSck:shutdown( 'write' )
			else
				seg:next()
				if cnt < chunk_sz then
					-- check for proper remainder size
					assert( cnt == #payload % chunk_sz, fmt('Sent chunk was #%d, expected %d', cnt, #payload % chunk_sz ) )
				else
					assert( cnt == chunk_sz, fmt('Sent chunk was #%d, expected %d', cnt, chunk_sz ) )
				end
			end
		end
		makeReceiver( self, #payload, payload, done )
		makeSender( self, sender )
	end,

	test_cb_sendBufferNonBlocking = function( self, done )
		Test.Case.describe( "cnt = sck.send( buf_seg ) -- on nonblocking socket" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 600000 )
		local buf    = Buffer( payload )
		local seg    = buf:Segment( )   -- cover entire Buffer
		local sender = function( s )
			local cnt = s.cSck:send( seg )
			if cnt > 0 then
				sendCount = sendCount+1
				outCount  = cnt + outCount
				if outCount < #buf then
					seg.start = outCount + 1
				else
					seg.start = outCount
					seg.size = 0
				end
			else
				assert( sendCount>1, fmt( "Non blocking should have broken up sending: %d ", sendCount) )
				assert( outCount  == #buf, fmt( "send() should accumulate to (%d) but sent(%d)", #buf, outCount ) )
				s.loop:removeHandle( s.cSck, 'write' )
				s.cSck:shutdown( 'write' )
			end
		end
		makeReceiver( self, #payload, payload, done )
		makeSender( self, sender, true )
	end,
}

return Test( tests )
