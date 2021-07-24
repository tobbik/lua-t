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
local Loop      = require( "t.Loop" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )
local Buffer    = require( "t.Buffer" )
local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr

local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr
local config    = t_require( "t_cfg" )

local pp = require't.Table'.pprint

-- #########################################################################
-- accept server for each test and set up recv()
local makeReceiver = function( self, size, payload )
	local inCount, incBuffer, rCnt = 0, Buffer( size ),0
	local recv = function( )
		local seg     = inCount < #incBuffer and incBuffer:Segment( inCount+1 ) or incBuffer:Segment( #incBuffer, 0 )
		local suc,cnt = self.rcvSck:recv( seg )
		rCnt = rCnt +1
		if suc then
			assert( incBuffer:read( inCount+1, cnt ) == payload:sub( inCount+1, cnt+inCount ),
				("%d[%d]Received and original should match:\n%s\n---------------------------\n%s"):format(
				inCount, cnt, incBuffer:read( inCount+1, cnt ), payload:sub( inCount+1, cnt+inCount  ) ) )
			inCount = cnt + inCount
		else
			Test.notes("ReceiveCount: %d", rCnt)
			--print("ReceiveCount:", rCnt)
			assert( inCount  ==  size, ("Send(%d) and Recv(%d) count should be equal"):format( size, inCount ) )
			assert( incBuffer:read() == payload, "Sent payload shouls equal received overall message" )
			self.loop:removeHandle( self.rcvSck, "read" )
			self.rcvSck:close( )
		end
	end
	local acpt = function( )
		self.rcvSck, self.rcvAdr = self.srvSck:accept( )
		assert( chkSck( self.rcvSck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.rcvAdr, "AF_INET", self.host, 'any' ) )
		self.loop:addHandle( self.rcvSck, "read", recv )
		self.loop:removeHandle( self.srvSck, "read" )
	end
	self.loop:addHandle( self.srvSck, "read", acpt )
end

local makeSender = function( self, sender, nonblock )
	self.sndSck, self.sndAdr = Socket.connect( self.srvAdr )
	assert( chkSck( self.sndSck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
	assert( chkAdr( self.sndAdr, "AF_INET", self.host, self.port ) )
	self.loop:addHandle( self.sndSck, 'write', sender, self )
	if nonblock then self.sndSck.nonblock = true end
	self.loop:run( )
end

return {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self )
		self.loop                = Loop( )
		self.host                = Interface.default( ).address.ip
		self.port                = config.nonPrivPort
		self.srvSck, self.srvAdr = Socket.listen( self.host, self.port )
		assert( chkSck( self.srvSck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.srvAdr, "AF_INET", self.host, self.port ) )
		--io.write("Sleeping ... ")
		--self.loop:sleep( 2000 )
		--print("Done ")
	end,

	afterAll = function( self )
		self.srvSck:close( )
	end,

	--[[
	--]]
	-- #########################################################################
	-- Actual Test cases
	sendFullString = function( self )
		Test.describe( "cnt = sck.send( msg ) -- send all in one go" )
		local payload = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 41000 )
		local sender  = function( s )
			assert( s.sndSck.sendbuffer >= #payload,
			        ("Sendbuffer[%d] is smaller than #msg[%d]. Consider adjusting size."):format( s.sndSck.sendbuffer, #payload ) )
			local cnt = s.sndSck:send( payload )
			assert( cnt == #payload, ("Blocking send() should send all(%d) but sent(%d)"):format( #payload, cnt ) )
			s.loop:removeHandle( s.sndSck, "write" )
			s.sndSck:close( )
		end
		makeReceiver( self, #payload, payload )
		makeSender( self, sender )
	end,

	sendStringSmallChunks = function( self )
		Test.describe( "cnt = sck.send( msg, sz ) -- small sized chunks" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 40000 )
		local chunk_sz = 128
		local sender = function( s )
			-- the substring only has to be longer than chunksize
			local cnt = s.sndSck:send( payload:sub( outCount+1, outCount+chunk_sz ), chunk_sz )
			if cnt>0 then
				if cnt < chunk_sz then
					-- check for proper remainder size
					assert( cnt == #payload % chunk_sz, ('Sent chunk was #%d, expected %d'):format( cnt, #payload % chunk_sz ) )
				else
					assert( cnt == chunk_sz, ('Sent chunk was #%d, expected %d'):format( cnt, chunk_sz ) )
				end
				sendCount = sendCount+1
				outCount  = cnt + outCount
			else
				assert( sendCount == math.ceil( outCount/chunk_sz ),
				        ("expected iterations: %d/%d"):format( sendCount, math.ceil( outCount/chunk_sz ) ) )
				assert( outCount  == #payload, ("send() should accumulate to (%d) but sent(%d)"):format( #payload, outCount ) )
				self.loop:removeHandle( self.sndSck, "write" )
				self.sndSck:close( )
			end
		end
		makeReceiver( self, #payload, payload )
		makeSender( self, sender )
	end,

	sendFullBuffer = function( self )
		Test.describe( "cnt = sck.send( buf ) -- send all in one go" )
		local payload  = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 41000 )
		local buf      = Buffer( payload )
		local sender = function( s )
			assert( s.sndSck.sendbuffer >= #payload,
			        ("Sendbuffer[%d] is smaller than #msg[%d]. Consider adjusting size."):format( s.sndSck.sendbuffer, #payload ) )
			local cnt = s.sndSck:send( buf )
			assert( cnt == #buf, ("Blocking send() should send all(%d) but sent(%d)"):format( #buf, cnt ) )
			s.loop:removeHandle( s.sndSck, 'write' )
			s.sndSck:close( )
		end
		makeReceiver( self, #payload, payload )
		makeSender( self, sender )
	end,

	sendBufferSmallChunks = function( self )
		Test.describe( "cnt = sck.send( buf_seg ) -- small sized chunks" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 50000 )
		local buf      = Buffer( payload )
		local chunk_sz = 128
		local seg      = buf:Segment( 1, chunk_sz )
		local sender   = function( s )
			local cnt = s.sndSck:send( seg )
			outCount  = outCount + cnt
			sendCount = sendCount+1
			if outCount == #buf then
				assert( sendCount == math.ceil( outCount/chunk_sz ),
				        ("expected iterations: %d/%d"):format( sendCount, math.ceil( outCount/chunk_sz ) ) )
				assert( outCount  == #buf, ("send() should accumulate to (%d) but sent(%d)"):format( #buf, outCount ) )
				s.loop:removeHandle( s.sndSck, "write" )
				s.sndSck:close( )
			else
				seg:next()
				if cnt < chunk_sz then
					-- check for proper remainder size
					assert( cnt == #payload % chunk_sz, ('Sent chunk was #%d, expected %d'):format( cnt, #payload % chunk_sz ) )
				else
					assert( cnt == chunk_sz, ('Sent chunk was #%d, expected %d'):format( cnt, chunk_sz ) )
				end
			end
		end
		makeReceiver( self, #payload, payload )
		makeSender( self, sender )
	end,

	sendStringNonBlocking = function( self )
		Test.describe( "cnt = sck.send( msg ) on nonblocking socket" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 600000 )
		local sender = function( s )
			local cnt = s.sndSck:send( payload:sub( outCount+1 ) )
			if cnt > 0 then
				sendCount = sendCount+1
				outCount  = cnt + outCount
			else
				assert( sendCount>1, ("Non blocking should have broken up sending: %d "):format( sendCount) )
				assert( outCount  == #payload, ("send() should accumulate to (%d) but sent(%d)"):format( #payload, outCount ) )
				s.loop:removeHandle( s.sndSck, "write" )
				s.sndSck:close( )
			end
		end
		makeReceiver( self, #payload, payload )
		makeSender( self, sender, true )
	end,

	sendBufferNonBlocking = function( self )
		Test.describe( "cnt = sck.send( buf_seg ) -- on nonblocking socket" )
		local sendCount, outCount, payload = 0, 0,
			string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 600000 )
		local buf    = Buffer( payload )
		local seg    = buf:Segment( )   -- cover entire Buffer
		local sender = function( s )
			local cnt = s.sndSck:send( seg )
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
				assert( sendCount>1, ("Non blocking should have broken up sending: %d "):format( sendCount) )
				assert( outCount  == #buf, ("send() should accumulate to (%d) but sent(%d)"):format( #buf, outCount ) )
				s.loop:removeHandle( s.sndSck, "write" )
				s.sndSck:close( )
			end
		end
		makeReceiver( self, #payload, payload )
		makeSender( self, sender, true )
	end,
}
