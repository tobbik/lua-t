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


return {
	beforeAll  = function( self )
		-- UNIX only
		-- TODO: make more universal
		local h = io.popen( 'id' )
		local s = h:read( '*a' )
		h:close()
		self.isPriv = not not s:match( 'uid=0' ) -- isPriviledged? root?
		self.host   = Interface.default( ).address.ip
		self.port   = 1502
	end,

	afterEach = function( self )
		if self.sck then
			self.sck:close( )
			self.sck = nil
		end
		self.adr = nil
	end,

	SocketBindCreateSockAndIanyAddress = function( self )
		Test.describe( "Socket.bind() --> creates a TCP IPv4 Socket and 0.0.0.0:0 address" )
		self.sck, self.adr = Socket.bind()
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM', self.adr ) )
		assert( chkAdr( self.adr, "AF_INET", '0.0.0.0', 0, self.adr ) )
	end,

	SocketBindPrivPortThrowsPermission = function( self )
		-- This Test behaves differently when run as Root (it succeeds)
		Test.describe( "Socket.bind( priviledged port ) --> throws permission error" )
		if self.isPriv then
			Test.skip( "Test is for unauthorized behaviour" )
		end
		local port   = config.privPort
		local errMsg = "Can't bind socket to 0.0.0.0:"..port.." %(Permission denied%)"
		local a,e    = Socket.bind( port )
		assert( not a, "Should fail binding to priviledged port as non root" )
		assert( e:match( errMsg ), ("Expected error message:\n%s\n%s"):format( errMsg, e ) )
	end,

	SocketBindPortCreateSockAndInanyAddress = function( self )
		Test.describe( "Socket.bind( port ) --> creates TCP IPv4 Socket and 0.0.0.0:port address" )
		self.sck, self.adr = Socket.bind( self.port )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM', self.adr ) )
		assert( chkAdr( self.adr, "AF_INET", '0.0.0.0', self.port, self.adr ) )
	end,

	SocketBindHostPortCreateSockAndAddress = function( self )
		Test.describe( "Socket.bind(host,port) --> creates TCP IPv4 Socket and address" )
		self.sck, self.adr = Socket.bind( self.host, self.port )
		assert( chkAdr( self.adr, "AF_INET", self.host, self.port, self.adr ) )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
	end,

	SocketBindAddressCreateSockOnly = function( self )
		Test.describe( "Socket.bind(address) --> creates TCP IPv4 Socket but no address" )
		local addr   = Address( self.host, self.port )
		self.sck, self.adr = Socket.bind( addr )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM', self.adr ) )
		assert( chkAdr( self.adr, "AF_INET", self.host, self.port, self.adr ) )
		assert( addr == self.adr, ("The returned address`%s` should equal input `%s`"):format( self.adr, addr ) )
	end,

	SocketBindReturnBoundSocket = function( self )
		Test.describe( "Socket.bind(address) --> returning socket is bound; getsockname()" )
		local addr   = Address( self.host, self.port )
		self.sck, self.adr = Socket.bind( addr )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM', self.adr ) )
		assert( chkAdr( self.adr, "AF_INET", self.host, self.port, self.adr ) )
		assert( addr  == self.sck:getsockname(), "The addresses should be equal" )
	end,

	-- ##################################################################
	-- Do the same test just on an existing socket s:bind( ... )
	-- ##################################################################

	socketBindCreateInAnyAddress = function( self )
		Test.describe( "s:bind() --> creates a 0.0.0.0:0 address" )
		self.sck     = Socket()
		local adr,b = self.sck:bind()
		assert( chkAdr( adr, "AF_INET", '0.0.0.0', 0, b ) )
	end,

	socketBindPortCreateInAnyAddress = function( self )
		Test.describe( "s:bind( port ) --> creates 0.0.0.0:port address" )
		self.sck     = Socket()
		local adr, b = self.sck:bind( self.port )
		assert( chkAdr( adr, "AF_INET", '0.0.0.0', self.port, b ) )
	end,

	socketBindHostPortCreateAddress = function( self )
		Test.describe( "s:bind(host,port) --> creates address" )
		self.sck     = Socket()
		local adr, b = self.sck:bind( self.host, self.port )
		assert( chkAdr( adr, "AF_INET", self.host, self.port, b ) )
	end,

	socketBindAddressCreateNothingButBinds = function( self )
		Test.describe( "s:bind(address) --> creates nothing but does bind" )
		local addr   = Address( self.host, self.port )
		self.sck     = Socket()
		local adr,b  = self.sck:bind( addr )
		assert( chkAdr( addr, "AF_INET", self.host, self.port, b ) )
		assert( addr  == self.sck:getsockname(), "The addresses should be equal" )
		assert( addr  == adr, "The addresses should be equal input" )
	end,

	socketBindWrongArgFails = function( self )
		Test.describe( "sck.bind( adr ) --> fails" )
		self.sck    = Socket()
		local eMsg  = "bad argument #1 to `bind` %(expected `t.Net.Socket`, got `nil`%)"
		local f     = function() local _,__ = self.sck.bind( ) end
		local ran,e = pcall( f )
		assert( not ran, "This should have failed" )
		assert( e:match( eMsg), ("Expected error message:\n%s\n%s"):format( eMsg:gsub('%%',''), e ) )
	end,

	socketCloseBoundNoSockname = function( self )
		Test.describe( "Can't call getsockname on closed socket" )
		local eMsg      = "Couldn't get peer address (Bad file descriptor)"
		self.sck        = Socket()
		local a,b       = self.sck:getsockname( )
		assert( chkAdr( a, "AF_INET", '0.0.0.0', 0 ) )
		self.sck:close( )
		a,b       = self.sck:getsockname( )
		assert( not a, ("getsockname() should not have returned a value but got `%s`"):format( a ) )
		assert( b:match( eMsg:gsub( "%(", "%%(" ):gsub( "%)", "%%)" ) ),
		   ("Error Message should have been `%s', but was `%s`"):format( eMsg, b ) )
	end,

	socketBindAlreadyBoundFails = function( self )
		Test.describe( "sck.bind( adr ) --> already bound to adr fails " )
		self.sck    = Socket()
		local addr  = Address( self.host, self.port )
		local eMsg  = ("Can't bind socket to %s:%s %%(Invalid argument%%)"):format(addr.ip, addr.port)
		local x,e   = self.sck:bind( addr )  -- success
		local x,e   = self.sck:bind( addr )  -- fail, already bound
		assert( not x, "bind() should have failed" )
		assert( e:match( eMsg), ("Expected error message:\n%s\n%s"):format( eMsg:gsub('%%',''), e ) )
	end,

}
