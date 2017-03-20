#!../out/bin/lua

--- \file    t_net_sck_connect.lua \brief   Test assuring Socket.connect(...)
--and socket:connect(...) handles all use cases \detail  All permutations tested
--in this suite: sck,_   = Socket.connect( ip )         -- Sck IPv4(TCP) sck,adr
--= Socket.connect( host, port ) -- Sck IPv4(TCP); Adr host:port _,__    =
--sck.connect( ip )         -- perform bind and listen adr,__  = sck.connect(
--host, port ) -- Adr host:port
--
-- These tests run asynchronously.  A TCP server socket is listening while each
-- test tries to connect to it.  This allows for these tests to run within the
-- same  process.  Each test will restart the loop, connect, assert and stop the
-- loop before moving on to the next test.

local T       = require( 't' )
local Test    = T.Test
local Timer   = T.Time
local Loop    = T.Loop
local Socket  = T.Net.Socket
local Address = T.Net.IPv4
local assrt   = T.require( 't_net_assert' )


-- #########################################################################
-- accept server for each test
accept = function( self )
	local c, ip = self.srv:accept( )
	assrt.Socket( c, 'tcp', 'AF_INET', 'SOCK_STREAM' )
	assrt.Address( ip, self.host2cmp, self.addr2cmp )
	self.sck:close() -- close client first to avoid "Address already in use"
	                 -- http://hea-www.harvard.edu/~fine/Tech/addrinuse.html
	c:close( )
end

local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.host          = T.Net.Interface( 'default' ).address:get()
		self.port          = 8000
		self.srv, self.adr = Socket.listen( self.host, self.port )
		print( self.srv, self.adr )
		self.loop          = Loop( 20 )
		self.loop:addHandle( self.srv, true, accept, self )
		done()
	end,

	afterAll = function( self, done )
		self.loop:removeHandle( self.srv, true )
		self.srv:close( )
		done()
	end,

	beforeEach_cb = function( self, done )
		self.loop:addTimer( Timer(1), done )
		-- loop:run() blocks further execution until the function on the loop
		-- runs the afterEach_cb and releases the block forcing all tests to be
		-- executed sequentially
		self.loop:run()
	end,
	
	afterEach_cb = function( self, done )
		self.sck:close( )
		self.loop:stop()
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_SListenHostPort = function( self, done )
		Test.Case.describe( "Socket.connect( host, port ) --> Sck IPv4(TCP), Adr host:port" )
		self.loop:addTimer( 
			Timer( 1 ),
			function(  )
				self.host2cmp = self.host
				self.addr2cmp = 'any'
				self.sck, self.adrc = Socket.connect( self.host, self.port )
				done( )
			end
		)
	end,

	test_cb_SListenAddress = function( self, done )
		Test.Case.describe( "Socket.connect( adr ) --> Sck IPv4(TCP)" )
		self.loop:addTimer( 
			Timer( 1 ),
			function(  )
				self.host2cmp    = self.host
				self.addr2cmp    = 'any'
				self.adrc        = Address( self.host, self.port )
				self.sck, self._ = Socket.connect( self.adrc )
				assert( self._ == nil, "Only a socket should have been returned" )
				done( )
			end
		)
	end,

	test_cb_sListenHostPort = function( self, done )
		Test.Case.describe( "sck:connect( host, port ) --> Adr host:port" )
		self.loop:addTimer( 
			Timer( 1 ),
			function(  )
				self.host2cmp    = self.host
				self.addr2cmp    = 'any'
				self.sck         = Socket( )
				self.adrc,self._ = self.sck:connect( self.host, self.port )
				assert( self._  == nil, "No socket should have been returned" )
				done( )
			end
		)
	end,

	test_cb_sListenAddress = function( self, done )
		Test.Case.describe( "sck:connect( adr ) --> no return value" )
		self.loop:addTimer( 
			Timer( 1 ),
			function(  )
				self.host2cmp    = self.host
				self.addr2cmp    = 'any'
				self.adrc        = Address( self.host, self.port )
				self.sck         = Socket( )
				self.__,self._   = self.sck:connect( self.adrc )
				assert( self.__ == nil, "No socket  should have been returned" )
				assert( self._  == nil, "No address should have been returned" )
				done( )
			end
		)
	end,

	test_cb_sListenHostPortBound = function( self, done )
		Test.Case.describe( "sck:connect( host, port ) bound --> Adr host:port" )
		self.loop:addTimer( 
			Timer( 1 ),
			function(  )
				-- bind socket to outgoing interface/port first 
				self.host2cmp       = self.host
				self.port2cmp       = 11111
				--self.sck,self.adrb  = Socket.bind( self.host2cmp, self.port2cmp )
				self.sck            = Socket()
				self.sck.reuseaddr  = true
				self.adrb           = self.sck:bind( self.host2cmp, self.port2cmp )
				self.adrc,self._    = self.sck:connect( self.host, self.port )
				assert( self._  == nil, "No socket should have been returned" )
				done( )
			end
		)
	end,

	test_cb_sListenAddressBound = function( self, done )
		Test.Case.describe( "sck:connect( adr ) bound --> no return value" )
		self.loop:addTimer( 
			Timer( 1 ),
			function(  )
				-- bind socket to outgoing interface/port first 
				self.host2cmp       = self.host
				self.port2cmp       = 11111
				--self.sck,self.adrb  = Socket.bind( self.host2cmp, self.port2cmp )
				self.sck            = Socket()
				self.sck.reuseaddr  = true
				self.adrb           = self.sck:bind( self.host2cmp, self.port2cmp )
				self.adrc           = Address( self.host, self.port )
				self.__,self._      = self.sck:connect( self.adrc )
				assert( self.__     == nil, "No socket  should have been returned" )
				assert( self._      == nil, "No address should have been returned" )
				done( )
			end
		)
	end,

}

t = Test( tests )
t( )
print( t )
