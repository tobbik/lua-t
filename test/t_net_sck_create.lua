#!../out/bin/lua

---
-- \file    t_net_sck_create.lua
-- \brief   Test assuring Socket() constructor handles all intended use cases

T       = require( 't' )
Test    = T.Test
Socket  = T.Net.Socket
Address = T.Net.IPv4

local tests = {
	assertSocket = function( self, sck, pro, fam, typ )
		assert( pro == sck.protocol, "Protocol should be "..pro.." but is ".. tostring(sck.protocol) )
		assert( fam == sck.family  , "Family should be "  ..fam.." but is ".. tostring(sck.family) )
		assert( typ == sck.type    , "Type should be "    ..typ.." but is ".. tostring(sck.type) )
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
		self.sock = Socket()
		self:assertSocket(  self.sock, 'tcp', 'AF_INET', 'SOCK_STREAM' )
	end,

	test_TcpArgsCreatesTcpSocket = function( self )
		Test.Case.describe( "Socket('TCP') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'TCP' )
		self:assertSocket(  self.sock, 'tcp', 'AF_INET', 'SOCK_STREAM' )
	end,

	test_LowerCaseProtocolArgs = function( self )
		Test.Case.describe( "Socket('tcp') --> creates a TCP IPv4 Socket" )
		self.sock = Socket( 'tcp' )
		self:assertSocket(  self.sock, 'tcp', 'AF_INET', 'SOCK_STREAM' )
	end,

	test_UdpArgsCreatesUdpSocket = function( self )
		Test.Case.describe( "Socket('UDP') --> creates a UDP IPv4 Socket" )
		self.sock = Socket( 'UDP' )
		self:assertSocket(  self.sock, 'udp', 'AF_INET', 'SOCK_DGRAM' )
	end,

	test_Ip6ArgsCreatesUdpSocket = function( self )
		Test.Case.describe( "Socket('UDP', 'AF_INET6') --> creates a UDP IPv6 Socket" )
		self.sock = Socket( 'UDP', 'Ip6', 'SOCK_DGRAM' )
		self:assertSocket(  self.sock, 'udp', 'AF_INET6', 'SOCK_DGRAM' )
	end,

	test_CreatesCongestionControlSocket = function( self )
		Test.Case.describe( "Socket('DCCP', 'ip6', 'SOCK_DCCP') --> creates a Congestion control IPv6 Socket" )
		self.sock =  Socket('DCCP', 'ip6', 'SOCK_DCCP' )
		self:assertSocket(  self.sock, 'dccp', 'AF_INET6', 'SOCK_DCCP' )
	end,

	test_CreatesWithIp4Aliases = function( self )
		Test.Case.describe( "Socket(*,'ip4', 'AF_INET4', 'IPv4') --> all create IPv4 Sockets" )
		for i,f in pairs( { "AF_INET", "ip4", "Ip4", "IP4", "IPv4" } ) do
			local sock = Socket( 'TCP', f )
			self:assertSocket(  sock, 'tcp', 'AF_INET', 'SOCK_STREAM' )
			sock:close()
		end
	end,

	test_CreatesWithIp6Aliases = function( self )
		Test.Case.describe( "Socket(*,'ip6', 'AF_INET6', 'IPv6') --> all create IPv6 Sockets" )
		for i,f in pairs( { "AF_INET6", "ip6", "Ip6", "IP6", "IPv6" } ) do
			local sock = Socket( 'TCP', f )
			self:assertSocket(  sock, 'tcp', 'AF_INET6', 'SOCK_STREAM' )
			sock:close()
		end
	end,

	test_CreatesWithStreamAliases = function( self )
		Test.Case.describe( "Socket(*, *, 'stream', ... ) --> all create Stream Sockets" )
		for i,t in pairs( { "SOCK_STREAM", "stream", "STREAM" } ) do
			local sock = Socket( 'TCP', 'ip4', t )
			self:assertSocket(  sock, 'tcp', 'AF_INET', 'SOCK_STREAM' )
			sock:close()
		end
	end,

	test_CreatesWithRawAliases = function( self )
		Test.Case.describe( "Socket(*, *, 'raw', ...) --> all create RAW Sockets" )
		Test.Case.skip( "Must be privileged user (root) to execute that" )
		for i,t in pairs( { "SOCK_RAW", "raw", "RAW" } ) do
			local sock = Socket( 'TCP', 'ip4', t )
			self:assertSocket(  sock, 'tcp', 'AF_INET', 'SOCK_RAW' )
			sock:close()
		end
	end,

	test_CreatesWithGramAliases = function( self )
		Test.Case.describe( "Socket(*, *, 'dgram', ...) --> all create DGRAM Sockets" )
		for i,t in pairs( { "SOCK_DGRAM", "dgram", "DGRAM", "Datagram", "datagram", "DATAGRAM" } ) do
			local sock = Socket( 'UDP', 'ip4', t )
			self:assertSocket(  sock, 'udp', 'AF_INET', 'SOCK_DGRAM' )
			sock:close()
		end
	end,

}

t = Test( tests )
t( )
print( t )
