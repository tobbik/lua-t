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

local t_require =   require( "t" ).require
local Test      =   require( "t.Test" )
local Socket    =   require( "t.Net.Socket" )
local Address   =   require( "t.Net.Address" )
local Interface =   require( "t.Net.Interface" )
local chkAdr    = t_require( "assertHelper" ).Adr
local chkSck    = t_require( "assertHelper" ).Sck
local config    = t_require( "t_cfg" )
local fmt       = string.format


local tests = {
	beforeAll  = function( self, done )
		-- UNIX only
		-- TODO: make more universal
		local h = io.popen( 'id' )
		local s = h:read( '*a' )
		h:close()
		self.isPriv = not not s:match( 'uid=0' ) -- isPriviledged? root?
		done( )
	end,

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
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.address, "AF_INET", '0.0.0.0', 0 ) )
	end,

	test_SBindPrivPortThrowsPermission = function( self )
		-- This Test behaves differently when run as Root (it succeeds)
		Test.Case.describe( "Socket.bind( priviledged port ) --> throws permission error" )
		if self.isPriv then
			Test.Case.skip( "Test is for unauthorized behaviour" )
		end
		local port   = config.privPort
		local errMsg = "Can't bind socket to 0.0.0.0:"..port.." %(Permission denied%)"
		local a,e    = Socket.bind( port )
		assert( not a, "Should fail binding to priviledged port as non root" )
		assert( e:match( errMsg ), fmt( "Expected error message:\n%s\n%s", errMsg, e ) )
	end,

	test_SBindPortCreateSockAndInanyAddress = function( self )
		Test.Case.describe( "Socket.bind( port ) --> creates TCP IPv4 Socket and 0.0.0.0:port address" )
		local port  = config.nonPrivPort
		self.sck, self.address = Socket.bind( port )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.address, "AF_INET", '0.0.0.0', port ) )
	end,

	test_SBindHostPortCreateSockAndAddress = function( self )
		Test.Case.describe( "Socket.bind(host,port) --> creates TCP IPv4 Socket and address" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = config.nonPrivPort
		self.sck, self.address = Socket.bind( host, port )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.address, "AF_INET", host, config.nonPrivPort ) )
	end,

	test_SBindAddressCreateSockOnly = function( self )
		Test.Case.describe( "Socket.bind(address) --> creates TCP IPv4 Socket but no address" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = config.nonPrivPort
		local addr   = Address( host, port )
		self.sck, self.adr = Socket.bind( addr )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( addr == self.adr, fmt( "The returned address`%s` should equal input `%s`", self.adr, addr ) )
	end,

	test_SBindReturnBoundSocket = function( self )
		Test.Case.describe( "Socket.bind(address) --> returning socket is bound; getsockname()" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = config.nonPrivPort
		local addr   = Address( host, port )
		self.sck, self.address = Socket.bind( addr )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
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
		assert( chkAdr( adr, "AF_INET", '0.0.0.0', 0 ) )
	end,

	test_sBindPortCreateInAnyAddress = function( self )
		Test.Case.describe( "s:bind( port ) --> creates 0.0.0.0:port address" )
		local port   = config.nonPrivPort
		self.sck     = Socket()
		local adr, b = self.sck:bind( port)
		assert( nil  == b, "The socket should not be returned" )
		assert( chkAdr( adr, "AF_INET", '0.0.0.0', config.nonPrivPort ) )
	end,

	test_sBindHostPortCreateAddress = function( self )
		Test.Case.describe( "s:bind(host,port) --> creates address" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = config.nonPrivPort
		self.sck     = Socket()
		local adr, b = self.sck:bind( host, port )
		assert( nil  == b, "The socket should not be returned" )
		assert( chkAdr( adr, "AF_INET", host, config.nonPrivPort ) )
	end,

	test_sBindAddressCreateNothingButBinds = function( self )
		Test.Case.describe( "s:bind(address) --> creates nothing but does bind" )
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = config.nonPrivPort
		local addr   = Address( host, port )
		self.sck     = Socket()
		local adr,b  = self.sck:bind( addr )
		assert( nil  == b,   "Only address should be returned" )
		assert( addr  == self.sck:getsockname(), "The addresses should be equal" )
		assert( addr  == adr, "The addresses should be equal input" )
		assert( chkAdr( addr, "AF_INET", host, port ) )
	end,

	test_sBindWrongArgFails = function( self )
		Test.Case.describe( "sck.bind( adr ) --> fails" )
		self.sck    = Socket()
		local eMsg  = "bad argument #1 to `bind` %(expected `t.Net.Socket`, got `nil`%)"
		local f     = function() local _,__ = self.sck.bind( ) end
		local ran,e = pcall( f )
		assert( not ran, "This should have failed" )
		assert( e:match( eMsg), fmt( "Expected error message:\n%s\n%s", eMsg:gsub('%%',''), e ) )
	end,

	test_sCloseBoundNoSockname = function( self )
		Test.Case.describe( "Can't call getsockname on closed socket" )
		local eMsg      = "Couldn't get Address from (Bad file descriptor)"
		self.sck        = Socket()
		local a,b       = self.sck:getsockname( )
		assert( chkAdr( a, "AF_INET", '0.0.0.0', 0 ) )
		self.sck:close( )
		a,b       = self.sck:getsockname( )
		assert( not a, fmt( "getsockname() should not have returned a value but got `%s`", a ) )
		assert( b:match( eMsg:gsub( "%(", "%%(" ):gsub( "%)", "%%)" ) ),
		   fmt( "Error Message should have been `%s', but was `%s`", eMsg, b ) )
	end,
	--]]
}

return Test( tests )
