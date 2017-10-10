# !../out/bin/lua

---
-- \file    t_net_sck_bind.lua
-- \brief   Test assuring Socket.bind() and socket:bind() handles all use cases
-- \details All tested permutations in this suite
--          Socket.bind()          --> creates a TCP IPv4 Socket and 0.0.0.0:0 address
--          Socket.bind(port)      --> creates TCP IPv4 Socket and 0.0.0.0:port address
--          Socket.bind(host,port) --> creates TCP IPv4 Socket and address
--          Socket.bind(address)   --> creates TCP IPv4 Socket but no address
--          Socket.bind(address)   --> returning socket is bound; getsockname()
--          s:bind()               --> creates a 0.0.0.0:0 address
--          s:bind(port)           --> creates 0.0.0.0:port address
--          s:bind(host,port)      --> creates address
--          s:bind(address)        --> creates nothing but does bind

local t_assert,t_require  =   require"t".assert, require"t".require
local Test      =   require( "t.Test" )
local Socket    =   require( "t.Net.Socket" )
local Address   =   require( "t.Net.Address" )
local Interface =   require( "t.Net.Interface" )
local asrtHlp   = t_require( "assertHelper" )

local tests = {

	afterEach = function( self )
		if self.sck then
			self.sck:close( )
			self.sck = nil
		end
		self.address = nil
	end,

	test_SBindCreateSockAndInanyAddress = function( self )
		Test.Case.describe( "Socket.bind() --> creates a TCP IPv4 Socket and 0.0.0.0:0 address" )
		self.sck, self.address = Socket.bind()
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.address, "AF_INET", '0.0.0.0', 0 )
	end,

	test_SBindPrivPortThrowsPermission = function( self )
		-- This Test behaves differently when run as Root (it succeeds)
		Test.Case.describe( "Socket.bind( priviledged port ) --> throws permission error" )
		local port   = 80
		local errMsg = "Can't bind socket to 0.0.0.0:"..port.." %(Permission denied%)"
		local a,e    = Socket.bind( port )
		assert( not a, "Should fail binding to priviledged port as non root" )
		t_assert( e:match( errMsg ), "Expected error message:\n%s\n%s", errMsg, e )
	end,

	test_SBindPortCreateSockAndInanyAddress = function( self )
		Test.Case.describe( "Socket.bind( port ) --> creates TCP IPv4 Socket and 0.0.0.0:port address" )
		local port  = 8000
		self.sck, self.address = Socket.bind( port )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.address, "AF_INET", '0.0.0.0', port )
	end,

	test_SBindHostPortCreateSockAndAddress = function( self )
		Test.Case.describe( "Socket.bind(host,port) --> creates TCP IPv4 Socket and address" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = 8000
		self.sck, self.address = Socket.bind( host, port )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.address, "AF_INET", host, 8000 )
	end,

	test_SBindAddressCreateSockOnly = function( self )
		Test.Case.describe( "Socket.bind(address) --> creates TCP IPv4 Socket but no address" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = 8000
		local addr   = Address( host, port )
		self.sck, self.adr = Socket.bind( addr )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		t_assert( addr == self.adr, "The returned address`%s` should equal input `%s`", self.adr, addr )
	end,

	test_SBindReturnBoundSocket = function( self )
		Test.Case.describe( "Socket.bind(address) --> returning socket is bound; getsockname()" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = 8000
		local addr   = Address( host, port )
		self.sck, self.address = Socket.bind( addr )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		assert( addr  == self.sck:getsockname(), "The addresses should be equal" )
	end,

	-- ##################################################################
	-- Do the same test just on an existing socket s:bind( ... )
	-- ##################################################################

	test_sBindCreateInAnyAddress = function( self )
		Test.Case.describe( "s:bind() --> creates a 0.0.0.0:0 address" )
		self.sck     = Socket()
		local adr,b = self.sck:bind()
		assert( nil  == b, "The socket should not be returned" )
		asrtHlp.Address( adr, "AF_INET", '0.0.0.0', 0 )
	end,

	test_sBindPortCreateInAnyAddress = function( self )
		Test.Case.describe( "s:bind( port ) --> creates 0.0.0.0:port address" )
		local port   = 8000
		self.sck     = Socket()
		local adr, b = self.sck:bind( port)
		assert( nil  == b, "The socket should not be returned" )
		asrtHlp.Address( adr, "AF_INET", '0.0.0.0', 8000 )
	end,

	test_sBindHostPortCreateAddress = function( self )
		Test.Case.describe( "s:bind(host,port) --> creates address" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = 8000
		self.sck     = Socket()
		local adr, b = self.sck:bind( host, port )
		assert( nil  == b, "The socket should not be returned" )
		asrtHlp.Address( adr, "AF_INET", host, 8000 )
	end,

	test_sBindAddressCreateNothingButBinds = function( self )
		Test.Case.describe( "s:bind(address) --> creates nothing but does bind" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = 8000
		local addr   = Address( host, port )
		self.sck     = Socket()
		local adr,b  = self.sck:bind( addr )
		assert( nil  == b,   "Only address should be returned" )
		assert( addr  == self.sck:getsockname(), "The addresses should be equal" )
		assert( addr  == adr, "The addresses should be equal input" )
		asrtHlp.Address( addr, "AF_INET", host, port )
	end,

	test_sBindWrongArgFails = function( self )
		Test.Case.describe( "sck.bind( adr ) --> fails" )
		self.sck        = Socket()
		local eMsg      = "bad argument #1 to 'bind' %(t.Net.Socket expected, got `nil`%)"
		local f     = function() local _,__ = self.sck.bind( ) end
		local ran,e = pcall( f )
		assert( not ran, "This should have failed" )
		t_assert( e:match( eMsg), "Expected error message:\n%s\n%s", eMsg:gsub('%%',''), e )
	end,

	test_sCloseBoundNoSockname = function( self )
		Test.Case.describe( "Can't call getsockname on closed socket" )
		local eMsg      = "Couldn't get Address from (Bad file descriptor)"
		self.sck        = Socket()
		local a,b       = self.sck:getsockname( )
		asrtHlp.Address( a, "AF_INET", '0.0.0.0', 0 )
		self.sck:close( )
		a,b       = self.sck:getsockname( )
		t_assert( not a, "getsockname() should not have returned a value but got `%s`", a )
		t_assert( b == eMsg, "Error Message should have been `%s', but was `%s`", eMsg, e )
	end,
	--]]
}

return Test( tests )
