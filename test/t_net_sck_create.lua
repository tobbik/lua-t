#!../out/bin/lua

---
-- \file    t_net_sck_create.lua
-- \brief   Test assuring Socket() constructor handles all intended use cases

local t_require,t_assert = require't'.require, require't'.assert
local Test    = require( "t.Test" )
local Socket  = require( "t.Net.Socket" )
local Address = require( "t.Net.Address" )
local asrtHlp = t_require( "assertHelper" )

local tests = {
	beforeAll  = function( self, done )
		-- UNIX only
		-- TODO: make more universal
		local h = io.popen( 'id' )
		local s = h:read( '*a' )
		h:close( )
		self.isPriv = not not s:match( 'uid=0' )
		done( )
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

	test_EmptyCreatesTcpIp4 = function( self )
		Test.Case.describe( "Socket() --> creates a TCP IPv4 Socket" )
		self.sock = Socket( )
		asrtHlp.Socket(  self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' )
	end,

	test_TcpArgsFulLCreatesTcpSocket = function( self )
		Test.Case.describe( "Socket('IPPROTO_TCP') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'IPPROTO_TCP' )
		asrtHlp.Socket(  self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' )
	end,

	test_TcpArgsCreatesTcpSocket = function( self )
		Test.Case.describe( "Socket('TCP') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'TCP' )
		asrtHlp.Socket(  self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' )
	end,

	test_LowerCaseProtocolArgs = function( self )
		Test.Case.describe( "Socket('tcp') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'tcp' )
		asrtHlp.Socket(  self.sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' )
	end,

	test_UdpArgsFullCreatesUdpSocket = function( self )
		Test.Case.describe( "Socket('IPPROTO_UDP') --> creates a UDP IPv4 Socket" )
		self.sock = Socket( 'IPPROTO_UDP' )
		asrtHlp.Socket(  self.sock, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' )
	end,

	test_UdpArgsCreatesUdpSocket = function( self )
		Test.Case.describe( "Socket('UDP') --> creates a UDP IPv4 Socket" )
		self.sock = Socket( 'UDP' )
		asrtHlp.Socket(  self.sock, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' )
	end,

	test_Ip6ArgsCreatesUdpSocket = function( self )
		Test.Case.describe( "Socket('UDP', 'AF_INET6') --> creates a UDP IPv6 Socket" )
		self.sock = Socket( 'UDP', 'Ip6', 'SOCK_DGRAM' )
		asrtHlp.Socket(  self.sock, 'IPPROTO_UDP', 'AF_INET6', 'SOCK_DGRAM' )
	end,

	test_CreatesControlMessageSocket = function( self )
		Test.Case.describe( "Socket('ICMP', 'ip6', 'SOCK_RAW') --> creates a control message  IPv6 Socket" )
		if not self.isPriv then
			Test.Case.skip( "Must be privileged user (root) to create ICMP Socket" )
		end
		self.sock =  Socket('ICMP', 'ip6', 'SOCK_RAW' )
		asrtHlp.Socket(  self.sock, 'IPPROTO_ICMP', 'AF_INET6', 'SOCK_RAW' )
	end,

	test_CreatesWithIp4Aliases = function( self )
		Test.Case.describe( "Socket(*,'ip4', 'AF_INET4', 'IPv4') --> all create IPv4 Sockets" )
		for i,f in pairs( { "AF_INET", "ip4", "Ip4", "IP4", "IPv4" } ) do
			local sock = Socket( 'TCP', f )
			asrtHlp.Socket(  sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' )
			sock:close()
		end
	end,

	test_CreatesWithIp6Aliases = function( self )
		Test.Case.describe( "Socket(*,'ip6', 'AF_INET6', 'IPv6') --> all create IPv6 Sockets" )
		for i,f in pairs( { "AF_INET6", "ip6", "Ip6", "IP6", "IPv6" } ) do
			self.sock = Socket( 'TCP', f )
			asrtHlp.Socket(  self.sock, 'IPPROTO_TCP', 'AF_INET6', 'SOCK_STREAM' )
			self.sock:close( )
		end
	end,

	test_CreatesWithStreamAliases = function( self )
		Test.Case.describe( "Socket(*, *, 'stream', ... ) --> all create Stream Sockets" )
		for i,t in pairs( { "SOCK_STREAM", "stream", "STREAM" } ) do
			local sock = Socket( 'TCP', 'ip4', t )
			asrtHlp.Socket(  sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_STREAM' )
			sock:close()
		end
	end,

	test_CreatesWithRawAliases = function( self )
		Test.Case.describe( "Socket(*, *, 'raw', ...) --> all create RAW Sockets" )
		if not self.isPriv then
			Test.Case.skip( "Must be privileged user (root) to create RAW Sockets" )
		end
		for i,t in pairs( { "SOCK_RAW", "raw", "RAW" } ) do
			local sock = Socket( 'TCP', 'ip4', t )
			asrtHlp.Socket(  sock, 'IPPROTO_TCP', 'AF_INET', 'SOCK_RAW' )
			sock:close()
		end
	end,

	test_CreatesWithGramAliases = function( self )
		Test.Case.describe( "Socket('udp', 'IPv4', '*dgram', ...) --> all create DGRAM Sockets" )
		for i,t in pairs( { "SOCK_DGRAM", "dgram", "DGRAM", "Datagram", "datagram", "DATAGRAM" } ) do
			self.sock = Socket( 'UDP', 'ip4', t )
			asrtHlp.Socket(  self.sock, 'IPPROTO_UDP', 'AF_INET', 'SOCK_DGRAM' )
			self.sock:close()
		end
	end,

	test_CreatesHasProperDescriptor = function( self )
		Test.Case.describe( "Socket( ).descriptor has proper values" )
		self.sock = Socket( )
		t_assert( self.sock.descriptor, "Socket descriptor should be avalid value" )
		t_assert( 'number' == type( self.sock.descriptor ), "Socket descriptor should be a `number` but was `%s`", type( self.sock.descriptor ) )
		t_assert( self.sock.descriptor > 0, "Socket descriptor should be bigger than 0 but was `%s`", self.sock.descriptor )
	end,

	test_ClosingSocketReleasesDescriptor = function( self )
		Test.Case.describe( "sck.descriptor should be `nil` after sck.close()" )
		self.sock = Socket( )
		assert( self.sock.descriptor, "Socket descriptor should be avalid value" )
		self.sock:close( )
		t_assert( nil == self.sock.descriptor, "Closed sockets descriptor should be `nil` but was `%s`", type( self.sock.descriptor ) )
		self.sock = nil -- make self.afterEach() happy
	end,
}

return Test( tests )
