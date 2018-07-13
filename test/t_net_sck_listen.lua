#!../out/bin/lua

---
-- \file    t_net_sck_bind.lua
-- \brief   Test assuring Socket.listen(...) and socket:listen(...) handles all use cases
-- \detail  All permutations tested in this suite:
--  sck,adr = Socket.listen(  )                    -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
--  sck,adr = Socket.listen( bl )                  -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
--  sck,adr = Socket.listen( adr )                 -- Sck IPv4(TCP)
--  sck,adr = Socket.listen( adr, bl )             -- Sck IPv4(TCP)
--  sck,adr = Socket.listen( host )                -- Sck IPv4(TCP); Adr host:(0)
--  sck,adr = Socket.listen( host, port )          -- Sck IPv4(TCP); Adr host:port
--  sck,adr = Socket.listen( host, port, bl )      -- Sck IPv4(TCP); Adr host:port
--  adr  = Socket.listen( sck )                 -- just listen; assume bound socket
--  adr  = Socket.listen( sck, bl )             -- just listen; assume bound socket
--  adr  = Socket.listen( sck, adr )            -- perform bind and listen
--  adr  = Socket.listen( sck, adr, bl )        -- perform bind and listen
--  adr  = Socket.listen( sck, host )           -- Adr host:xxxxx
--  adr  = Socket.listen( sck, host, port )     -- Adr host:port
--  adr  = Socket.listen( sck, host, port, bl ) -- Adr host:port


local Test      = require( "t.Test" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )

local t_require = require( "t" ).require
local chkSck    = t_require( "assertHelper" ).Sck
local chkAdr    = t_require( "assertHelper" ).Adr
local fmt       = string.format
local config    = t_require( "t_cfg" )

local tests = {
	beforeAll  = function( self, done )
		-- UNIX only
		-- TODO: make more universal
		local h = io.popen( 'id' )
		local s = h:read( '*a' )
		h:close()
		self.isPriv = not not s:match( 'uid=0' )
		done( )
	end,

	beforeEach = function( self )
	end,

	afterEach = function( self )
		if self.sck then
			self.sck:close( )
			self.sck = nil
		end
		self.adr, self._, self.__ = nil, nil, nil
	end,

	test_SListenEmpty = function( self )
		Test.Case.describe( "Socket.listen( ) --> Sck IPv4(TCP); Adr 0.0.0.0:xxxxx" )
		self.sck, self.adr = Socket.listen( )
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", '0.0.0.0', 'any' ) )
		assert( self.sck:getsockname() == self.adr, fmt( "Bound[ %s ] and returned [ %s ]address should match",
			self.sck:getsockname(), self.adr ) )
	end,

	test_SListenBacklog = function( self )
		Test.Case.describe( "Socket.listen( 5 ) --> Sck IPv4(TCP); Adr 0.0.0.0:xxxxx" )
		self.sck,self.adr  = Socket.listen( 5 )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", '0.0.0.0', 'any' ) )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,

	test_SListenAddress = function( self )
		Test.Case.describe( "Socket.listen( adr ) --> Sck IPv4(TCP)" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = config.nonPrivPort
		local addr        = Address( host, port )
		self.sck,self.adr = Socket.listen( addr )
		assert( self.adr == addr, "Returned Address shall be same as input" )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( self.sck:getsockname() == addr, "Bound address should equal input" )
	end,

	test_SListenAddressBacklog = function( self )
		Test.Case.describe( "Socket.listen( adr, backlog ) --> Sck IPv4(TCP)" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = config.nonPrivPort
		local addr        = Address( host, port )
		self.sck,self.adr = Socket.listen( addr, 5 )
		assert( self.adr == addr, "Returned Address shall be same as input" )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( self.sck:getsockname() == addr, "Bound address should equal input" )
	end,

	test_SListenHost = function( self )
		Test.Case.describe( "Socket.listen( host ) --> Sck IPv4(TCP), Adr host:xxxxx" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		self.sck,self.adr = Socket.listen( host )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", host, 'any' ) )
		assert( chkAdr( self.sck:getsockname(), "AF_INET", host, 'any' ) )
	end,

	test_SListenHostPort = function( self )
		Test.Case.describe( "Socket.listen( host,port ) --> Sck IPv4(TCP), Adr host:port" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = config.nonPrivPort
		self.sck,self.adr = Socket.listen( host, port )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", host, port ) )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,

	test_SListenPortBacklog = function( self )
		Test.Case.describe( "Socket.listen( port, bl ) --> Sck IPv4(TCP), Adr 0.0.0.0:port" )
		local port        = config.nonPrivPort
		self.sck,self.adr = Socket.listen( port, 50 )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", '0.0.0.0', port ) )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,

	test_SListenHostPrivPortThrowsPermission = function( self )
		-- This Test behaves differently when run as Root (it succeeds)
		Test.Case.describe( "Socket.listen( host, priviledged port ) --> throws permission error" )
		if self.isPriv then
			Test.Case.skip( "Test is for unauthorized behaviour" )
		end
		local host   = Interface( 'default' ).AF_INET.address.ip
		local port   = config.privPort
		local errMsg = "Can't bind socket to "..host..":"..port.." %(Permission denied%)"
		local f,e    = Socket.listen( host, port )
		assert( not f, "Should have failed as non-root user" )
		assert( e:match( errMsg ), fmt( "Expected Error Message: `%s` but got `%s`", errMsg, e ) )
	end,

	test_SListenHostAddressBacklog = function( self )
		Test.Case.describe( "Socket.listen( host,port,backlog ) --> Sck IPv4(TCP), Adr host:port" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = config.nonPrivPort
		self.sck,self.adr = Socket.listen( host, port, 5 )
		assert( chkSck(  self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", host, port ) )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,

	-- ##################################################################
	-- Do the same test just on an existing socket s:listen( ... )
	-- ##################################################################
	test_sListen = function( self )
		Test.Case.describe( "sck:listen( ) --> void (Assume Socket is already bound)" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = config.nonPrivPort
		self.sck,self.adr = Socket.bind( host, port )
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", host, port ) )
		local a, b        = self.sck:listen( )
		assert( b == nil, "No second value should have been returned" )
		assert( self.adr == a, fmt( "returned address `%s` should equal previously bound `%s`", a, self.adr ) )
		assert( self.sck:getsockname() == self.adr,
			fmt( "Bound address should equal input `%s` but was `%s`", self.adr, self.sck:getsockname() ) )
	end,

	test_sListenBacklog = function( self )
		Test.Case.describe( "sck:listen( backlog ) --> void (Assume Socket is already bound)" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = config.nonPrivPort
		self.sck,self.adr = Socket.bind( host, port )
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.adr, "AF_INET", host, port ) )
		local a, b        = self.sck:listen( 5 )
		assert( b == nil, "No second value should have been returned" )
		assert( a == self.adr, fmt( "returned address `%s` should equal previously bound `%s`", a, self.adr ) )
		assert( self.sck:getsockname() == self.adr,
			fmt( "Bound address should equal input `%s` but was `%s`", self.adr, self.sck:getsockname() ) )
	end,

	test_sListenAddress = function( self )
		Test.Case.describe( "sck:listen( adr ) --> void" )
		local host = Interface( 'default' ).AF_INET.address.ip
		local port = config.nonPrivPort
		self.adr   = Address( host, port )
		self.sck   = Socket( )
		local a, b = self.sck:listen( self.adr )
		assert( b == nil, "No second value should have been returned" )
		assert( a == self.adr, "Returned address should be same as input address" )
		assert( self.sck:getsockname() == self.adr,
			fmt( "Bound address should equal input `%s` but was `%s`", self.adr, self.sck:getsockname() ) )
	end,

	test_sListenAddressBacklog = function( self )
		Test.Case.describe( "sck:listen( adr, backlog ) --> void" )
		local host = Interface( 'default' ).AF_INET.address.ip
		local port = config.nonPrivPort
		self.adr   = Address( host, port )
		self.sck   = Socket( )
		local a, b = self.sck:listen( self.adr, 5 )
		assert( b == nil, "No second value should have been returned" )
		assert( a == self.adr, "Returned address should be same as input address" )
		assert( self.sck:getsockname() == self.adr,
			fmt( "Bound address should equal input `%s` but was `%s`", self.adr, self.sck:getsockname() ) )
	end,

	test_sListenHost = function( self )
		Test.Case.describe( "sck:listen( host ) --> Adr host:xxxxx" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		self.sck        = Socket( )
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		local a, b      = self.sck:listen( host )
		assert( b == nil, "No second value should have been returned" )
		assert( chkAdr( a, "AF_INET", host, 'any' ) )
		assert( self.sck:getsockname() == a,
			fmt( "Bound address should equal input `%s` but was `%s`", self.adr, self.sck:getsockname() ) )
	end,

	test_sListenHostPort = function( self )
		Test.Case.describe( "sck:listen( host, port ) --> Adr host:port" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = config.nonPrivPort
		self.sck        = Socket( )
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		local a,b       = self.sck:listen( host, port )
		assert( b == nil, "No values should have been returned" )
		assert( chkAdr( a, "AF_INET", host, port ) )
		assert( self.sck:getsockname() == a,
			fmt( "Bound address should equal input `%s` but was `%s`", a, self.sck:getsockname() ) )
	end,

	test_sListenPortBacklog = function( self )
		Test.Case.describe( "sck:listen( port, bl ) --> Adr 0.0.0.0:port" )
		local port      = config.nonPrivPort
		self.sck        = Socket( )
		self.adr        = self.sck:listen( port, 50 )
		assert( chkAdr( self.adr, "AF_INET", '0.0.0.0', port ) )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,

	test_sListenHostPortBacklog = function( self )
		Test.Case.describe( "sck:listen( host, port, backlog ) --> Adr host:port" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = config.nonPrivPort
		self.sck        = Socket( )
		assert( chkSck( self.sck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		local a,b       = self.sck:listen( host, port, 5 )
		assert( b == nil, "No second value should have been returned" )
		assert( chkAdr( a, "AF_INET", host, port ) )
		assert( self.sck:getsockname() == a,
			fmt( "Bound address should equal input `%s` but was `%s`", a, self.sck:getsockname() ) )
	end,

	test_sListenWrongArgFails = function( self )
		Test.Case.describe( "sck.listen( adr ) --> fails" )
		self.sck    = Socket()
		local eMsg  = "bad argument #1 to `listen` %(expected `t.Net.Socket`, got `nil`%)"
		local f     = function() local _,__ = self.sck.listen( ) end
		local ran,e = pcall( f )
		assert( not ran, "This should have failed" )
		assert( e:match( eMsg), fmt( "Expected error message:\n`%s`\nbut got `%s`\n\n", eMsg:gsub('%%',''), e ) )
	end,
--]]
}

return Test( tests )
