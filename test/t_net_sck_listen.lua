#!../out/bin/lua

---
-- \file    t_net_sck_bind.lua
-- \brief   Test assuring Socket.listen(...) and socket:listen(...) handles all use cases
-- \detail  All permutations tested in this suite:
--          sck,adr = Socket.listen(  )                    -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
--          sck,adr = Socket.listen( bl )                  -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
--          sck,_   = Socket.listen( ip )                  -- Sck IPv4(TCP)
--          sck,_   = Socket.listen( ip, bl )              -- Sck IPv4(TCP)
--          sck,adr = Socket.listen( host )                -- Sck IPv4(TCP); Adr host:(0)
--          sck,adr = Socket.listen( host, port )          -- Sck IPv4(TCP); Adr host:port
--          sck,adr = Socket.listen( host, port, bl )      -- Sck IPv4(TCP); Adr host:port
--          _,__    = Socket.listen( sck )                 -- just listen; assume bound socket
--          _,__    = Socket.listen( sck, bl )             -- just listen; assume bound socket
--          _,__    = Socket.listen( sck, ip )             -- perform bind and listen
--          _,__    = Socket.listen( sck, ip, bl )         -- perform bind and listen
--          adr,__  = Socket.listen( sck, host )           -- Adr host:xxxxx
--          adr,__  = Socket.listen( sck, host, port )     -- Adr host:port
--          adr,__  = Socket.listen( sck, host, port, bl ) -- Adr host:port


local T         = require( "t" )
local Test      = require( "t.Test" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local Interface = require( "t.Net.Interface" )
local asrtHlp   = T.require( "assertHelper" )

local tests = {

	beforeEach = function( self )
	end,

	afterEach = function( self )
		if self.sck then
			self.sck:close( )
			self.sck = nil
		end
		self.address, self._, self.__ = nil, nil, nil
	end,

	test_SListenEmpty = function( self )
		Test.Case.describe( "Socket.listen( ) --> Sck IPv4(TCP); Adr 0.0.0.0:xxxxx" )
		self.sck,self.adr  = Socket.listen( )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.adr, "AF_INET", '0.0.0.0', 'any' )
		T.assert( self.sck:getsockname() == self.adr, "Bound[ %s ] and returned [ %s ]address should match",
		self.sck:getsockname(), self.adr )
	end,

	test_SListenBacklog = function( self )
		Test.Case.describe( "Socket.listen( 5 ) --> Sck IPv4(TCP); Adr 0.0.0.0:xxxxx" )
		self.sck,self.adr  = Socket.listen( 5 )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.adr, "AF_INET", '0.0.0.0', 'any' )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,

	test_SListenAddress = function( self )
		Test.Case.describe( "Socket.listen( adr ) --> Sck IPv4(TCP)" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = 8000
		local addr      = Address( host, port )
		self.sck,self._ = Socket.listen( addr )
		assert( self._ == nil, "Only a socket should have been returned" )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		assert( self.sck:getsockname() == addr, "Bound address should equal input" )
	end,

	test_SListenAddressBacklog = function( self )
		Test.Case.describe( "Socket.listen( adr, backlog ) --> Sck IPv4(TCP)" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = 8000
		local addr      = Address( host, port )
		self.sck,self._ = Socket.listen( addr, 5 )
		assert( self._ == nil, "Only a socket should have been returned" )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		assert( self.sck:getsockname() == addr, "Bound address should equal input" )
	end,

	test_SListenHost = function( self )
		Test.Case.describe( "Socket.listen( host ) --> Sck IPv4(TCP), Adr host:xxxxx" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		self.sck,self.adr = Socket.listen( host )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.adr, "AF_INET", host, 'any' )
		asrtHlp.Address( self.sck:getsockname(), "AF_INET", host, 'any' )
	end,

	test_SListenHostAddress = function( self )
		Test.Case.describe( "Socket.listen( host,port ) --> Sck IPv4(TCP), Adr host:port" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = 8000
		self.sck,self.adr = Socket.listen( host, port )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.adr, "AF_INET", host, port )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,

	test_SListenHostPrivPortThrowsPermission = function( self )
		-- This Test behaves differently when run as Root (it succeeds)
		Test.Case.describe( "Socket.listen( host, priviledged port ) --> throws permission error" )
		local host  = Interface( 'default' ).AF_INET.address.ip
		local port  = 80
		local f     = function(self, host) self.sck, self.adr = Socket.listen( host, 80 ) end
		local ran,e = pcall( f, self, host )
		assert( not ran, "Don't run tests a root" )
		assert( e:match( "Can't bind socket to "..host..":"..port.." before listen%(%) %(Permission denied%)" ),
			  "Error message\n"..
			  "should be:".."Can't bind socket to "..host..":"..port.." before listen() (Permission denied)"..
			  "\n  but was: "..e )
	end,

	test_SListenHostAddressBacklog = function( self )
		Test.Case.describe( "Socket.listen( host,port,backlog ) --> Sck IPv4(TCP), Adr host:port" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = 8000
		self.sck,self.adr = Socket.listen( host, port, 5 )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.adr, "AF_INET", host, port )
		assert( self.sck:getsockname() == self.adr, "Bound and returned address should match" )
	end,


	-- ##################################################################
	-- Do the same test just on an existing socket s:listen( ... )
	-- ##################################################################

	test_sListen = function( self )
		Test.Case.describe( "sck.listen( ) --> void (Assume Socket is already bound)" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = 8000
		self.sck,self.adr = Socket.bind( host, port )
		asrtHlp.Socket( self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.adr, "AF_INET", host, port )
		self._,self.__    = self.sck:listen( )
		assert( self._  == nil, "No values should have been returned" )
		assert( self.__ == nil, "No values should have been returned" )
		assert( self.sck:getsockname() == self.adr, "Bound address should equal input" )
	end,

	test_sListenBacklog = function( self )
		Test.Case.describe( "sck.listen( backlog ) --> void (Assume Socket is already bound)" )
		local host        = Interface( 'default' ).AF_INET.address.ip
		local port        = 8000
		self.sck,self.adr = Socket.bind( host, port )
		asrtHlp.Socket( self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		asrtHlp.Address( self.adr, "AF_INET", host, port )
		self._,self.__    = self.sck:listen( 5 )
		assert( self._  == nil, "No values should have been returned" )
		assert( self.__ == nil, "No values should have been returned" )
		assert( self.sck:getsockname() == self.adr, "Bound address should equal input" )
	end,

	test_sListenAddress = function( self )
		Test.Case.describe( "sck.listen( adr ) --> void" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = 8000
		self.adr        = Address( host, port )
		self.sck        = Socket( )
		self._,self.__  = self.sck:listen( self.adr )
		assert( self._  == nil, "No values should have been returned" )
		assert( self.__ == nil, "No values should have been returned" )
		assert( self.sck:getsockname() == self.adr, "Bound address should equal input" )
	end,

	test_sListenAddressBacklog = function( self )
		Test.Case.describe( "sck.listen( adr, backlog ) --> void" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = 8000
		self.adr        = Address( host, port )
		self.sck        = Socket( )
		self._,self.__  = self.sck:listen( self.adr, 5 )
		assert( self._  == nil, "No values should have been returned" )
		assert( self.__ == nil, "No values should have been returned" )
		assert( self.sck:getsockname() == self.adr, "Bound address should equal input" )
	end,

	test_sListenHost = function( self )
		Test.Case.describe( "sck.listen( host ) --> Adr host:xxxxx" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		self.sck        = Socket( )
		asrtHlp.Socket(  self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		self.adr,self._ = self.sck:listen( host )
		assert( self._  == nil, "No values should have been returned" )
		asrtHlp.Address( self.adr, "AF_INET", host, 'any' )
		assert( self.sck:getsockname() == self.adr, "Bound address should equal input" )
	end,

	test_sListenHostPort = function( self )
		Test.Case.describe( "sck.listen( host, port ) --> Adr host:port" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = 8000
		self.sck        = Socket( )
		asrtHlp.Socket( self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		self.adr,self._ = self.sck:listen( host, port )
		assert( self._  == nil, "No values should have been returned" )
		asrtHlp.Address( self.adr, "AF_INET", host, port )
		assert( self.sck:getsockname() == self.adr, "Bound address should equal input" )
	end,

	test_sListenHostPortBacklog = function( self )
		Test.Case.describe( "sck.listen( host, port, backlog ) --> Adr host:port" )
		local host      = Interface( 'default' ).AF_INET.address.ip
		local port      = 8000
		self.sck        = Socket( )
		asrtHlp.Socket( self.sck, 'tcp', 'AF_INET', 'SOCK_STREAM' )
		self.adr,self._ = self.sck:listen( host, port, 5 )
		assert( self._  == nil, "No values should have been returned" )
		asrtHlp.Address( self.adr, "AF_INET", host, port )
		assert( self.sck:getsockname() == self.adr, "Bound address should equal input" )
	end,

	test_sListenWrongArgFails = function( self )
		Test.Case.describe( "sck.listen( adr ) --> fails" )
		self.sck        = Socket()
		local eMsg      = "bad argument #1 to 'listen' %(T.Net.Socket expected, got no value%)"
		local f     = function() local _,__ = self.sck.listen( ) end
		local ran,e = pcall( f )
		assert( not ran, "This should have failed" )
		T.assert( e:match( eMsg), "Expected error message:\n%s\n%s", eMsg:gsub('%%',''), e )
	end,

}

return Test( tests )
