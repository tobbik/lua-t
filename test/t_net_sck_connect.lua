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

local t_require = require"t".require
local Test      = require( "t.Test" )
local Loop      = require( "t.Loop" )
local Interface = require( "t.Net.Interface" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local chkAdr    = t_require( "assertHelper" ).Adr
local chkSck    = t_require( "assertHelper" ).Sck
local config    = t_require( "t_cfg" )


-- #########################################################################
-- accept server for each test

local kick = function() end
return {
	-- #########################################################################
	-- wrappers for tests
	beforeAll = function( self )
		self.host               = Interface.default( ).address.ip
		self.port, self.portAlt = 1500, 1501
		--self.srvSck, self.srvAdr = Socket.listen( self.host, self.port )
		self.srvSck, self.srvAdr = Socket(), Address( self.host, self.port )
		self.srvSck.reuseaddr, self.srvSck.reuseport = true,true
		self.srvSck:listen( self.srvAdr )
		assert( chkSck( self.srvSck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
		assert( chkAdr( self.srvAdr, "AF_INET", self.host, self.port ) )
		self.loop          = Loop( )
	end,

	afterAll = function( self )
		self.srvSck:close( )
	end,

	beforeEach = function( self )
		local accept = function( s )
			s.rcvSck, s.rcvAdr = s.srvSck:accept( )
			--print("ACCEPTED REQUEST ON:", s.rcvAdr, s.rcvSck:getsockname())
			assert( chkSck( s.rcvSck, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
			assert( chkAdr( s.rcvAdr, "AF_INET", s.host2cmp, s.port2cmp ) )
			s.loop:removeHandle( s.srvSck, "read" )
			s.rcvSck:close( )
			s.sndSck:close( ) -- close client first to avoid "Address already in use"
								   -- http://hea-www.harvard.edu/~fine/Tech/addrinuse.html
		end
		self.loop:addHandle( self.srvSck, "read", accept, self )
	end,

	--[[
	-- #########################################################################
	-- Actual Test cases
	ConnectHostPort = function( self )
		Test.describe( "Socket.connect( host, port ) --> Sck IPv4(TCP), Adr host:port" )
		self.host2cmp = self.host
		self.port2cmp = 'any'
		self.sndSck, self.sndAdr = Socket.connect( self.host, self.port )
		self.loop:run()
	end,

	ConnectAddress = function( self )
		Test.describe( "Socket.connect( adr ) --> Sck IPv4(TCP)" )
		self.host2cmp    = self.host
		self.port2cmp    = 'any'
		local adrc       = Address( self.host, self.port )
		self.sndSck, self.sndAdr = Socket.connect( adrc )
		assert( chkAdr( self.sndAdr, adrc, _ ) )
		self.loop:run()
	end,

	connectHostPort = function( self )
		Test.describe( "sck:connect( host, port ) --> Adr host:port" )
		self.host2cmp    = self.host
		self.port2cmp    = 'any'
		self.sndSck      = Socket( )
		local adrc       = Address( self.host, self.port )
		self.sndAdr, _   = self.sndSck:connect( self.host, self.port )
		assert( chkAdr( self.sndAdr, "AF_INET", self.host, self.port, _ ) )
		assert( _  == nil, "No socket should have been returned" )
		self.loop:run()
	end,

	connectAddress = function( self )
		Test.describe( "sck:connect( adr ) --> no return value" )
		self.host2cmp   = self.host
		self.port2cmp   = 'any'
		local adrc      = Address( self.host, self.port )
		self.sndSck     = Socket( )
		self.sndAdr,_   = self.sndSck:connect( adrc )
		assert( chkAdr( self.sndAdr, adrc, _ ) )
		assert( _ == nil, "No socket  should have been returned" )
		self.loop:run()
	end,
	--]]

	connectHostPortBound = function( self )
		Test.describe( "sck:connect( host, port ) bound --> Adr host:port" )
		-- bind socket to outgoing interface/port first
		self.host2cmp         = self.host
		self.port2cmp         = self.portAlt
		self.sndSck           = Socket()
		local adrb,_          = self.sndSck:bind( self.host2cmp, self.port2cmp )
		assert( chkAdr( adrb, "AF_INET", self.host2cmp, self.port2cmp, _ ) )
		self.sndAdr, _        = self.sndSck:connect( self.host, self.port )
		assert( chkAdr( self.sndAdr, "AF_INET", self.host, self.port, _ ) )
		assert( _  == nil, "No socket should have been returned" )
		self.loop:run()
	end,

	connectAddressBound = function( self )
		Test.describe( "sck:connect( adr ) bound --> no return value" )
		-- bind socket to outgoing interface/port first
		self.host2cmp         = self.host
		self.port2cmp         = self.portAlt
		self.sndSck           = Socket()
		local adrb,_          = self.sndSck:bind( self.host2cmp, self.port2cmp )
		assert( chkAdr( adrb, "AF_INET", self.host2cmp, self.port2cmp, _ ) )
		local adrc            = Address( self.host, self.port )
		self.sndAdr, _        = self.sndSck:connect( adrc )
		assert( chkAdr( self.sndAdr, adrc, _ ) )
		assert( _ == nil, ("Only address should have been returned, but returned <%s>"):format( _ ) )
		self.loop:run()
	end,

	--[[
	connectWrongArgFails = function( self )
		Test.describe( "sck.connect( adr ) --> fails" )
		self.sndSck = Socket( )
		local eMsg  = "bad argument #1 to `connect` %(expected `t.Net.Socket`, got `nil`%)"
		local f     = function( )
			local _,__ = self.sndSck.connect( )
		end
		local ran,e = pcall( f )
		assert( not ran, "This should have failed" )
		assert( e:match( eMsg), ("Expected vs receievd error message:\n%s\n%s"):format( eMsg:gsub('%%',''), e ) )
		self.sndSck:close( )
	end,
	--]]
}

