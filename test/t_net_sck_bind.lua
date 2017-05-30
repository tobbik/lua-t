# !../out/bin/lua

---
-- \file    t_net_sck_bind.lua
-- \brief   Test assuring Socket.bind() and socket:bind() handles all use cases
-- \details All tested permutations in this suite
--          Socket.bind() --> creates a TCP IPv4 Socket and 0.0.0.0:0 address
--          Socket.bind( port ) --> creates TCP IPv4 Socket and 0.0.0.0:port address
--          Socket.bind(host,port) --> creates TCP IPv4 Socket and address
--          Socket.bind(address) --> creates TCP IPv4 Socket but no address
--          Socket.bind(address) --> returning socket is bound; getsockname()
--          s:bind() --> creates a 0.0.0.0:0 address
--          s:bind( port ) --> creates 0.0.0.0:port address
--          s:bind(host,port) --> creates address
--          s:bind(address) --> creates nothing but does bind

local T         = require( "t" )
local Test      = require( "t.Test" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )
local asrtHlp   = T.require( "assertHelper" )

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
		asrtHlp.Address( self.address, '0.0.0.0', 0 )
	end,

	test_SBindPrivPortThrowsPermission = function( self )
		-- This Test behaves differently when run as Root (it succeeds)
		Test.Case.describe( "Socket.bind( priviledged port ) --> throws permission error" )
		local port  = 80
		local f     = function() self.sck, self.address = Socket.bind( port ) end
		local ran,e = pcall( f )
		assert( not ran, "Don't run tests a root" )

		T.assert( e:match( "Can't bind socket to 0.0.0.0:"..port.." %(Permission denied%)" ),
			"Expected error message:\n%s\n%s", "Can't bind socket to 0.0.0.0:"..port.." (Permission denied)", e )
	end,

	test_SBindPortCreateSockAndInanyAddress = function( self )
		Test.Case.describe( "Socket.bind( port ) --> creates TCP IPv4 Socket and 0.0.0.0:port address" )
		local port  = 8000
		self.sck, self.address = Socket.bind( port )
		local ip,prt = self.address:get()
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.address, '0.0.0.0', port )
	end,

	test_SBindHostPortCreateSockAndAddress = function( self )
		Test.Case.describe( "Socket.bind(host,port) --> creates TCP IPv4 Socket and address" )
		local host   = Interface( 'default' ).address:get()
		local port   = 8000
		self.sck, self.address = Socket.bind( host, port )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.address, host, 8000 )
	end,

	test_SBindAddressCreateSockOnly = function( self )
		Test.Case.describe( "Socket.bind(address) --> creates TCP IPv4 Socket but no address" )
		local host   = Interface( 'default' ).address:get()
		local port   = 8000
		local addr   = Address( host, port )
		self.sck, self.__ = Socket.bind( addr )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		assert( nil  == self.__, "The address should not be returned" )
	end,

	test_SBindReturnBoundSocke = function( self )
		Test.Case.describe( "Socket.bind(address) --> returning socket is bound; getsockname()" )
		local host   = Interface( 'default' ).address:get()
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
		self.address, self._ = self.sck:bind()
		assert( nil  == self._, "The socket should not be returned" )
		asrtHlp.Address( self.address, '0.0.0.0', 0 )
	end,

	test_sBindPortCreateInAnyAddress = function( self )
		Test.Case.describe( "s:bind( port ) --> creates 0.0.0.0:port address" )
		local port           = 8000
		self.sck             = Socket()
		self.address, self._ = self.sck:bind( port)
		assert( nil  == self._, "The socket should not be returned" )
		asrtHlp.Address( self.address, '0.0.0.0', 8000 )
	end,

	test_sBindHostPortCreateAddress = function( self )
		Test.Case.describe( "s:bind(host,port) --> creates address" )
		local host           = Interface( 'default' ).address:get()
		local port           = 8000
		self.sck             = Socket()
		self.address, self._ = self.sck:bind( host, port )
		assert( nil  == self._, "The socket should not be returned" )
		asrtHlp.Address( self.address, host, 8000 )
	end,

	test_sBindAddressCreateNothingButBinds = function( self )
		Test.Case.describe( "s:bind(address) --> creates nothing but does bind" )
		local host      = Interface( 'default' ).address:get()
		local port      = 8000
		local addr      = Address( host, port )
		self.sck        = Socket()
		self._, self.__ = self.sck:bind( addr )
		assert( nil  == self._ , "The socket  should not be returned" )
		assert( nil  == self.__, "The address should not be returned" )
		assert( addr  == self.sck:getsockname(), "The addresses should be equal" )
		asrtHlp.Address( addr, host, port )
	end,
}

return Test( tests )
