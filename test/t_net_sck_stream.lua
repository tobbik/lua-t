#!../out/bin/lua

---
-- \file    t_net_sck_stream
-- \brief   Test assuring s.send(...) work
-- \detail  Send and receive data via SOCK_STREAM sockets. All permutations
--          tested in this suite:
--                   s:snd( str )
--                   s:snd( buf )
--                   s:snd( buf_seg )
--                   s:snd( str, size )
--                   s:snd( buf, size )
--                   s:snd( buf_seg, size )
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
assrt   = T.require( 't_net_assert' )

-- #########################################################################
-- receive a chunk of data server for each test
receive = function( self )
	local msg,cnt,rcvdMsg;
	if self.incBuffer then
		msg,cnt = self.aSck:recv( self.incBuffer )
		--print(msg,cnt)
		if msg then
			rcvdMsg = self.incBuffer:read( 1, cnt )
		end
	else
		msg,cnt = self.aSck:recv( )
		--print(cnt)
		if msg then rcvdMsg = msg end
	end
	if msg then
		assert( rcvdMsg == self.payload:sub( self.inCount+1, cnt+self.inCount ),
			"Received and original should match" )
		self.inCount = cnt + self.inCount
	else
		assert( self.inCount  == self.outCount, "Send and Recv count should be equal" )
		assert( self.outCount == #self.payload,
			"Should have sent "..#self.payload.." bytes, but sent "..self.outCount )
		self.done( )
	end
end


-- #########################################################################
-- accept server for each test
accept = function( self )
	--print("ACCEPT")
	self.aSck, self.aAdr = self.sSck:accept( )
	--print( self.aSck, self.aAdr, receive )
	assrt.Socket( self.aSck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
	assrt.Address( self.aAdr, self.host, 'any' )
	self.loop:addHandle( self.aSck, 'read', receive, self )
end

local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.loop            = Loop( 20 )
		self.host            = T.Net.Interface( 'default' ).address:get( )
		self.port            = 8000
		self.sSck, self.sAdr = Socket.listen( self.host, self.port )
		self.loop:addHandle( self.sSck, 'read', accept, self )
		done()
	end,

	afterAll = function( self, done )
		self.loop:removeHandle( self.sSck, 'read' )
		self.sSck:close( )
		done()
	end,

	beforeEach_cb = function( self, done )
		self.loop:addTimer( Timer( 1 ), done )
		-- loop:run() blocks further execution until the function on the loop
		-- runs the afterEach_cb and releases the block forcing all tests to be
		-- executed sequentially
		--print(self.host, self.port)
		self.cSck, self.cAdr = Socket.connect( self.host, self.port )
		--print(self.cSck, self.cAdr)
		self.inCount   = 0
		self.outCount  = 0
		self.incBuffer = nil
		self.loop:run()
	end,

	afterEach_cb = function( self, done )
		self.loop:removeHandle( self.aSck, 'read' )
		self.cSck:close( )
		self.aSck:close( )
		self.loop:stop( )
		self.payload = nil
		--self.inCount  = 0
		--self.outCount = 0
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_sendFullString = function( self, done )
		Test.Case.describe( "cnt = sck.send( msg )" )
		self.payload = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 10000 )
		local sender = function( s )
			local cnt = s.cSck:send( s.payload )
			assert( cnt == #self.payload, "Blocking should have sent in single iteration" )
			s.outCount = cnt
			s.cSck:shutdown( 'write' )
			s.loop:removeHandle( s.cSck, 'write' )
		end
		self.done = done
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendStringSmallChunks = function( self, done )
		Test.Case.describe( "cnt = sck.send( msg, sz ) -- small sized chunks" )
		self.payload = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 10000 )
		local sendCount = 0
		local sender = function( s )
			local cnt = s.cSck:send( s.payload:sub( s.outCount+1 ), 128 )
			if cnt then
				sendCount = sendCount+1
				s.outCount = cnt + s.outCount
			else
				--print("SENT:", s.outCount)
				assert( sendCount == math.ceil(s.outCount/128),
					"expected iterations: "..sendCount.." / ".. math.ceil(s.outCount/128))
				s.cSck:shutdown( 'write' )
				s.loop:removeHandle( s.cSck, 'write' )
			end
		end
		self.done = done
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendStringNonBlocking = function( self, done )
		Test.Case.describe( "cnt = sck.send( msg ) on nonblocking socket" )
		self.payload = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 600000 )
		local sendCount = 0
		local sender = function( s )
			s.cSck.nonblock = true
			local cnt = s.cSck:send( s.payload:sub( s.outCount+1 ) )
			if cnt then
				sendCount = sendCount+1
				s.outCount = cnt + s.outCount
			else
				--print("SENT:", s.outCount)
				assert( sendCount>1, "Non blocking should have broken up sending "..sendCount)
				s.cSck:shutdown( 'write' )
				s.loop:removeHandle( s.cSck, 'write' )
			end
		end
		self.done = done
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendFullBuffer = function( self, done )
		Test.Case.describe( "cnt = sck.send( buf ) and receive via buffer" )
		self.payload   = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 10000 )
		local buf      = T.Buffer( self.payload )
		self.incBuffer = T.Buffer( 1024*50 )
		local sender = function( s )
			local cnt = s.cSck:send( buf )
			assert( cnt == #buf, "Blocking should have sent in single iteration" )
			s.outCount = cnt
			s.cSck:shutdown( 'write' )
			s.loop:removeHandle( s.cSck, 'write' )
		end
		self.done = done
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendBufferSmallChunks = function( self, done )
		Test.Case.describe( "cnt = sck.send( buf_seg ) -- small sized chunks" )
		self.payload    = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 10000 )
		local buf       = T.Buffer( self.payload )
		self.incBuffer  = T.Buffer( 500 )   -- rediculously small incBuffer ... should still work
		local sendCount = 0
		local sender = function( s )
			local seg = s.outCount+1+128 > #buf and T.Buffer.Segment( buf, s.outCount+1 )
			                                    or  T.Buffer.Segment( buf, s.outCount+1, 128 )
			local cnt = s.cSck:send( seg )
			if cnt then
				sendCount  = sendCount+1
				s.outCount = cnt + s.outCount
			else
				assert( sendCount == math.ceil(s.outCount/128),
					"expected iterations: "..sendCount.." / ".. math.ceil(s.outCount/128))
				s.cSck:shutdown( 'write' )
				s.loop:removeHandle( s.cSck, 'write' )
			end
		end
		self.done = done
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendBufferSizedChunks = function( self, done )
		Test.Case.describe( "cnt = sck.send( buf_seg, sz ) -- sized chunks" )
		self.payload    = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 10000 )
		local buf       = T.Buffer( self.payload )
		self.incBuffer  = T.Buffer( 1024*16 )   -- rediculously small incBuffer ... should still work
		local sendCount = 0
		local sender = function( s )
			local sz  = s.outCount+1+128 > #buf and #buf - s.outCount  or 128
			local cnt = s.cSck:send( T.Buffer.Segment( buf, s.outCount+1 ), sz )
			if cnt then
				sendCount  = sendCount+1
				s.outCount = cnt + s.outCount
			else
				assert( sendCount == math.ceil(s.outCount/128),
					"expected iterations: "..sendCount.." / ".. math.ceil(s.outCount/128))
				s.cSck:shutdown( 'write' )
				s.loop:removeHandle( s.cSck, 'write' )
			end
		end
		self.done = done
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

	test_cb_sendBufferNonBlocking = function( self, done )
		Test.Case.describe( "cnt = sck.send( buf_seg ) on nonblocking socket" )
		self.payload    = string.rep( 'THis Is a LittLe Test-MEsSage To bE sEnt ACcroSS the WIrE ...!_', 600000 )
		local buf       = T.Buffer( self.payload )
		self.incBuffer  = T.Buffer( 1024*64 )
		local sendCount = 0
		local sender    = function( s )
			s.cSck.nonblock = true
			local cnt = s.cSck:send( T.Buffer.Segment( buf, s.outCount+1 ) )
			if cnt then
				sendCount  = sendCount+1
				s.outCount = cnt + s.outCount
			else
				--print("SENT:", s.outCount)
				assert( sendCount>1, "Non blocking should have broken up sending "..sendCount)
				s.cSck:shutdown( 'write' )
				s.loop:removeHandle( s.cSck, 'write' )
			end
		end
		self.done = done
		self.loop:addHandle( self.cSck, 'write', sender, self )
	end,

}

t = Test( tests )
t( )
print( t )
