#!../out/bin/lua

---
-- \file    t_net_sck_dgram.lua
-- \brief   Test assuring s.send(...) work
-- \detail  Send and receive data via SOCK_STREAM sockets. All permutations
--          tested in this suite:
--                   s:snd( adr, str )
--                   s:snd( adr, buf )
--                   s:snd( adr, buf_seg )
--                   s:snd( adr, str, sz )
--                   s:snd( adr, buf, sz )
--                   s:snd( adr, buf_seg, sz )
--
-- These tests run (semi-)asynchronously.  A TCP server socket is listening while each
-- test connect it's own client to it.  This allows for these tests to run within the
-- same  process.  Each test will restart the loop, connect, assert and stop the
-- loop before moving on to the next test.

T       = require( 't' )
Test    = T.Test
Timer   = T.Time
Loop    = T.Loop
Socket  = T.Net.Socket
Address = T.Net.IPv4
Buffer  = T.Buffer
Segment = T.Buffer.Segment
assrt   = T.require( 't_net_assert' )

-- #########################################################################
-- receive a chunk of data server for each test
receive = function( self )
	local msg,cnt,rcvdMsg;
	local cAdr = Address();
	if self.incBuffer then
		msg,cnt = self.sSck:recv( cAdr, self.incBuffer )
		--print(msg,cnt)
		if msg then
			rcvdMsg = self.incBuffer:read( 1, cnt )
		end
	else
		msg,cnt = self.sSck:recv( cAdr )
		--print(cnt,cAdr)
		if msg then rcvdMsg = msg end
	end
	--print(cAdr,cnt)
	assrt.Address( cAdr, self.host, 'any' )
	if msg then
		assert( cnt == self.msgSize )
	else
		print("DONE")
		self.loop:removeHandle( self.sSck, 'read' )
		self.done( )
	end
end


local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop  = Loop( 20 )
		self.host  = T.Net.Interface( 'default' ).address:get( )
		self.port  = 8000
		self.sSck  = Socket( 'udp' )
		self.sAdr  = self.sSck:bind( self.host, self.port )
		--print(self.sSck, self.sAdr)
		-- self.loop:addHandle( self.sSck, 'read', receive, self )
		assrt.Socket( self.sSck, 'udp', 'AF_INET', 'SOCK_DGRAM' )
		assrt.Address( self.sAdr, self.host, self.host )
		self.cSck = Socket( 'udp' )
		self.cAdr = self.cSck:bind( self.host, self.port+1 )
		--print(self.cSck, self.cAdr)
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
		--print(self.host, self.port)
		self.inCount   = 0
		self.outCount  = 0
		self.incBuffer = nil
		print("BEFOREACH", done)
		self.loop:run()
	end,

	afterEach_cb = function( self, done )
		self.loop:stop( )
		--self.payload = nil
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_sendFullString = function( self, done )
		Test.Case.describe( "cnt = sck.send( adr, string )" )
		self.done    = done
		self.payload = string.rep( 'Gbivebvbes; RclcTctOCC ;h ;ie ea', 16 )
		self.msgSize = #self.payload
		local sender = function( s )
			s.loop:addHandle( s.sSck, 'read', receive, s )
			local cnt = s.cSck:send( s.sAdr, s.payload )
			assert( cnt == #self.payload, "Dgram should sent whole message at once" )
			s.loop:removeHandle( s.cSck, 'write' )
				print("SENTINEL SENT")
			local cnt = s.cSck:send( s.sAdr, '' ) --sentinel
		end
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendSizedString = function( self, done )
		Test.Case.describe( "cnt = sck.send( adr, string, sz )" )
		self.done    = done
		self.msgSize = 512
		self.payload = string.rep('This is a another test message for Dgram Socket se', self.msgSize )
		local sender = function( s )
			s.loop:addHandle( s.sSck, 'read', receive, s )
			local cnt = s.cSck:send( s.sAdr, s.payload:sub( s.outCount+1 ), s.msgSize )
			if cnt then
				assert( cnt == s.msgSize, "Dgram should sent whole sized message at once but was: "..cnt )
				s.outCount = s.outCount+cnt
			else
				s.loop:removeHandle( s.cSck, 'write' )
				local cnt = s.cSck:send( s.sAdr, '' ) --sentinel
				print("SENTINEL SENT")
			end
		end
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendFullBuffer = function( self, done )
		Test.Case.describe( "cnt = sck.send( adr, buf )" )
		self.done      = done
		self.payload   = string.rep( 'Gbivebvbes; RclcTctOCC ;h ;ie ea', 16 )
		self.outBuffer = Buffer( self.payload )
		self.msgSize   = #self.payload
		self.incBuffer = Buffer( #self.payload )
		local sender = function( s )
			s.loop:addHandle( s.sSck, 'read', receive, s )
			local cnt = s.cSck:send( s.sAdr, s.outBuffer )
			assert( cnt == #self.payload, "Dgram should sent whole message at once" )
			s.loop:removeHandle( s.cSck, 'write' )
				print("SENTINEL SENT")
			local cnt = s.cSck:send( s.sAdr, '' ) --sentinel
		end
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendSizedbuffer = function( self, done )
		Test.Case.describe( "cnt = sck.send( adr, buf, sz )" )
		self.done      = done
		self.msgSize   = 512
		self.payload   = string.rep('This is a another test message for Dgram Socket se', self.msgSize*50 )
		self.outBuffer = Buffer( self.payload )
		self.incBuffer = Buffer( self.msgSize )
		local sender = function( s )
			s.loop:addHandle( s.sSck, 'read', receive, s )
			local cnt = s.cSck:send( s.sAdr, Segment( s.outBuffer, s.outCount+1 ), s.msgSize )
			if cnt then
				assert( cnt == s.msgSize, "Dgram should sent whole sized message at once but was: "..cnt )
				s.outCount = s.outCount+cnt
			else
				s.loop:removeHandle( s.cSck, 'write' )
				local cnt = s.cSck:send( s.sAdr, '' ) --sentinel
				print("SENTINEL SENT")
			end
		end
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,
}

t = Test( tests )
t( )
print( t )
