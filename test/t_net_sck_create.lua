---
-- \file    t_net_sck_create.lua
-- \brief   Test assuring Socket() constructor handles all intended use cases

local t_require = require't'.require
local Test      = require( "t.Test" )
local Socket    = require( "t.Net.Socket" )
local Address   = require( "t.Net.Address" )
local chkSck    = t_require( "assertHelper" ).Sck

return {
	beforeAll  = function( self )
		-- UNIX only
		-- TODO: make more universal
		local h = io.popen( 'id' )
		local s = h:read( '*a' )
		h:close( )
		self.isPriv = not not s:match( 'uid=0' )
	end,

	beforeEach = function( self )
		-- print("SOURCE:", debug.getinfo(1, "S").source)
		-- print("ARGS:", arg[-3], arg[-2], arg[-1], arg[0], arg[1], arg[2] )
	end,

	afterEach = function( self )
		if self.sock then
			self.sock:close( )
			self.sock = nil
		end
		self.address = nil
	end,

	EmptyCreatesTcpIp4 = function( self )
		Test.describe( "Socket() --> creates a TCP IPv4 Socket" )
		self.sock = Socket( )
		assert( chkSck( self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
	end,

	TcpArgsFulLCreatesTcpSocket = function( self )
		Test.describe( "Socket('IPPROTO_TCP') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'IPPROTO_TCP' )
		assert( chkSck( self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
	end,

	TcpArgsCreatesTcpSocket = function( self )
		Test.describe( "Socket('TCP') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'TCP' )
		assert( chkSck( self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
	end,

	LowerCaseProtocolArgs = function( self )
		Test.describe( "Socket('tcp') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'tcp' )
		assert( chkSck( self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
	end,

	UdpArgsFullCreatesUdpSocket = function( self )
		Test.describe( "Socket('IPPROTO_UDP') --> creates a UDP IPv4 Socket" )
		self.sock = Socket( 'IPPROTO_UDP' )
		assert( chkSck( self.sock, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' ) )
	end,

	UdpArgsCreatesUdpSocket = function( self )
		Test.describe( "Socket('UDP') --> creates a UDP IPv4 Socket" )
		self.sock = Socket( 'UDP' )
		assert( chkSck( self.sock, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' ) )
	end,

	Ip6ArgsCreatesUdpSocket = function( self )
		Test.describe( "Socket('UDP', 'AF_INET6') --> creates a UDP IPv6 Socket" )
		self.sock = Socket( 'UDP', 'Ip6', 'SOCK_DGRAM' )
		assert( chkSck( self.sock, 'IPPROTO_UDP', 'AF_INET6', 'SOCK_DGRAM' ) )
	end,

	CreatesControlMessageSocket = function( self )
		Test.describe( "Socket('ICMP', 'ip6', 'SOCK_RAW') --> creates a control message  IPv6 Socket" )
		if not self.isPriv then
			Test.skip( "Must be privileged user (root) to create ICMP Socket" )
		end
		self.sock =  Socket('ICMP', 'ip6', 'SOCK_RAW' )
		assert( chkSck( self.sock, 'IPPROTO_ICMP', 'AF_INET6', 'SOCK_RAW' ) )
	end,

	CreatesWithIp4Aliases = function( self )
		Test.describe( "Socket(*,'ip4', 'AF_INET4', 'IPv4') --> all create IPv4 Sockets" )
		for i,f in pairs( { "AF_INET", "ip4", "Ip4", "IP4", "IPv4" } ) do
			local sock = Socket( 'TCP', f )
			assert( chkSck( sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
			sock:close()
		end
	end,

	CreatesWithIp6Aliases = function( self )
		Test.describe( "Socket(*,'ip6', 'AF_INET6', 'IPv6') --> all create IPv6 Sockets" )
		for i,f in pairs( { "AF_INET6", "ip6", "Ip6", "IP6", "IPv6" } ) do
			self.sock = Socket( 'TCP', f )
			assert( chkSck( self.sock, 'IPPROTO_TCP', 'AF_INET6', 'SOCK_STREAM' ) )
			self.sock:close( )
		end
	end,

	CreatesWithStreamAliases = function( self )
		Test.describe( "Socket(*, *, 'stream', ... ) --> all create Stream Sockets" )
		for i,t in pairs( { "SOCK_STREAM", "stream", "STREAM" } ) do
			local sock = Socket( 'TCP', 'ip4', t )
			assert( chkSck( sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' ) )
			sock:close()
		end
	end,

	CreatesWithRawAliases = function( self )
		Test.describe( "Socket(*, *, 'raw', ...) --> all create RAW Sockets" )
		if not self.isPriv then
			Test.skip( "Must be privileged user (root) to create RAW Sockets" )
		end
		for i,t in pairs( { "SOCK_RAW", "raw", "RAW" } ) do
			local sock = Socket( 'TCP', 'ip4', t )
			assert( chkSck( sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_RAW' ) )
			sock:close()
		end
	end,

	CreatesWithGramAliases = function( self )
		Test.describe( "Socket('udp', 'IPv4', '*dgram', ...) --> all create DGRAM Sockets" )
		for i,t in pairs( { "SOCK_DGRAM", "dgram", "DGRAM" } ) do
			self.sock = Socket( 'UDP', 'ip4', t )
			assert( chkSck( self.sock, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' ) )
			self.sock:close()
		end
	end,

	CreatesHasProperDescriptor = function( self )
		Test.describe( "Socket( ).descriptor has proper values" )
		self.sock = Socket( )
		assert( self.sock.descriptor, "Socket descriptor should be avalid value" )
		assert( 'number' == type( self.sock.descriptor ), ("Socket descriptor should be a `number` but was `%s`"):format( type( self.sock.descriptor ) ) )
		assert( self.sock.descriptor > 0, ("Socket descriptor should be bigger than 0 but was `%s`"):format( self.sock.descriptor ) )
	end,

	ClosingSocketReleasesDescriptor = function( self )
		Test.describe( "sck.descriptor should be `nil` after sck.close()" )
		self.sock = Socket( )
		assert( self.sock.descriptor, "Socket descriptor should be avalid value" )
		self.sock:close( )
		assert( nil == self.sock.descriptor, ("Closed sockets descriptor should be `nil` but was `%s`"):format( type( self.sock.descriptor ) ) )
		self.sock = nil -- make self.afterEach() happy
	end,
}
