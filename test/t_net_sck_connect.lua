#!../out/bin/lua

---
-- \file    t_net_sck_connect.lua
-- \brief   Test assuring Socket.connect(...) and socket:connect(...) handles all use cases
-- \detail  All permutations tested in this suite:
--          sck,_   = Socket.connect( ip )  Sck IPv4(TCP)
--          sck,adr = Socket.connect( host, port ) Sck IPv4(TCP); Adr host:port
--          _,__    = sck.connect( ip )         -- perform bind and listen
--          adr,__  = sck.connect( host, port ) -- Adr host:port
--
-- These tests run asynchronously.  A TCP server socket is listening while each
-- test tries to connect to it.  This allows for these tests to run within the
-- same  process.  Each test will restart the loop, connect, assert and stop the
-- loop before moving on to the next test.

local t_assert,t_require  =   require"t".assert, require"t".require
local Test      = require( "t.Test" )
local Timer     = require( "t.Time" )
local Loop      = require( "t.Loop" )
local Interface = require( "t.Net.Interface" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local asrtHlp   = t_require( "assertHelper" )


-- #########################################################################
-- accept server for each test
accept = function( self )
	local c, ip = self.srv:accept( )
	asrtHlp.Socket( c, 'tcp', 'AF_INET', 'SOCK_STREAM' )
	asrtHlp.Address( ip, "AF_INET", self.host2cmp, self.port2cmp )
	self.sck:close() -- close client first to avoid "Address already in use"
	                 -- http://hea-www.harvard.edu/~fine/Tech/addrinuse.html
	c:close( )
end

local tests = {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self, done )
		self.host          = Interface( 'default' ).AF_INET.address.ip
		self.port          = 8000
		self.srv, self.adr = Socket.listen( self.host, self.port )
		print( self.srv, self.adr )
		self.loop          = Loop( )
		self.loop:addHandle( self.srv, 'read', accept, self )
		done()
	end,
	afterAll = function( self, done )
		self.loop:removeHandle( self.srv, 'read' )
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
		self.loop:stop()
		done( )
	end,

	-- #########################################################################
	-- Actual Test cases
	test_cb_SConnectHostPort = function( self, done )
		Test.Case.describe( "Socket.connect( host, port ) --> Sck IPv4(TCP), Adr host:port" )
		self.host2cmp = self.host
		self.port2cmp = 'any'
		self.sck, self.adrc = Socket.connect( self.host, self.port )
		done( )
	end,

	test_cb_SConnectAddress = function( self, done )
		Test.Case.describe( "Socket.connect( adr ) --> Sck IPv4(TCP)" )
		self.host2cmp    = self.host
		self.port2cmp    = 'any'
		self.adrc        = Address( self.host, self.port )
		self.sck, self.a = Socket.connect( self.adrc )
		assert( self.a == self.adrc, "Returned address `%s` should equal input `%s`", self.a, self.adrc )
		done( )
	end,

	test_cb_sConnectHostPort = function( self, done )
		Test.Case.describe( "sck:connect( host, port ) --> Adr host:port" )
		self.host2cmp    = self.host
		self.port2cmp    = 'any'
		self.sck         = Socket( )
		self.adrc,self._ = self.sck:connect( self.host, self.port )
		assert( self._  == nil, "No socket should have been returned" )
		done( )
	end,

	test_cb_sConnectAddress = function( self, done )
		Test.Case.describe( "sck:connect( adr ) --> no return value" )
		self.host2cmp    = self.host
		self.port2cmp    = 'any'
		self.adrc        = Address( self.host, self.port )
		self.sck         = Socket( )
		local adr,b      = self.sck:connect( self.adrc )
		assert( b == nil, "No socket  should have been returned" )
		assert( adr == self.adrc, "Returned address `%s` should equal input `%s`", adr, self.adrc )
		done( )
	end,

	test_cb_sConnectHostPortBound = function( self, done )
		Test.Case.describe( "sck:connect( host, port ) bound --> Adr host:port" )
		-- bind socket to outgoing interface/port first 
		self.host2cmp       = self.host
		self.port2cmp       = 11111
		self.sck            = Socket()
		self.sck.reuseaddr  = true
		self.adrb           = self.sck:bind( self.host2cmp, self.port2cmp )
		self.adrc,self._    = self.sck:connect( self.host, self.port )
		assert( self._  == nil, "No socket should have been returned" )
		done( )
	end,

	test_cb_sConnectAddressBound = function( self, done )
		Test.Case.describe( "sck:connect( adr ) bound --> no return value" )
		-- bind socket to outgoing interface/port first 
		self.host2cmp       = self.host
		self.port2cmp       = 11111
		self.sck            = Socket()
		self.sck.reuseaddr  = true
		self.adrb           = self.sck:bind( self.host2cmp, self.port2cmp )
		self.adrc           = Address( self.host, self.port )
		local adr, b        = self.sck:connect( self.adrc )
		assert( b == nil, "Only address should have been returned" )
		t_assert( adr == self.adrc, "Returned address `%s` should equal input `%s`", adr, self.adrc )
		done( )
	end,

	test_cb_sConnectWrongArgFails = function( self, done )
		Test.Case.describe( "sck.connect( adr ) --> fails" )
		self.sck    = Socket( )
		local eMsg  = "bad argument #1 to 'connect' %(t.Net.Socket expected, got `nil`%)"
		local f     = function() local _,__ = self.sck.connect( ) end
		local ran,e = pcall( f )
		assert( not ran, "This should have failed" )
		t_assert( e:match( eMsg), "Expected error message:\n%s\n%s", eMsg:gsub('%%',''), e )
		done( )
	end,
--]]
}

return  Test( tests )
