#!../out/bin/lua

---
-- \file    test/t_net_adr.lua
-- \brief   Test the API of Net.Address

local T         =   require( "t" )
local Test      =   require( "t.Test" )
local Socket    =   require( "t.Net.Socket" )
local Address   =   require( "t.Net.Address" )
local Interface =   require( "t.Net.Interface" )
local asrtHlp   = T.require( "assertHelper" )
local fmt       = string.format

local tests = {

	test_CreateEmptyAddressIp4 = function( self )
		Test.Case.describe( "Socket.Address() --> creates an IPv4 Address {0.0.0.0:0}" )
		local adr = Address( )
		asrtHlp.Address( adr, "AF_INET", '0.0.0.0', 0 ) -- AF_INET is default
	end,

	test_CreateEmptyAddressIp6 = function( self )
		Test.Case.describe( "Socket.Address('::') --> creates an IPv6 Address {[::]:0}" )
		local adr = Address( '::' )
		asrtHlp.Address( adr, "AF_INET6", '::', 0 ) -- AF_INET6 inferred from IP6 Address
	end,

	test_CreateLocalhostAddressIp4 = function( self )
		Test.Case.describe( "Socket.Address( '127.0.0.1' ) --> creates localhost IPv4 Address {127.0.0.1:0}" )
		local adr = Address( '127.0.0.1' )
		asrtHlp.Address( adr, "AF_INET", '127.0.0.1', 0 ) -- AF_INET is default
	end,

	test_CreateLocalhostAddressIp6 = function( self )
		Test.Case.describe( "Socket.Address('::') --> creates an IPv6 Address {[::]:0}" )
		Test.Case.describe( "Socket.Address( '::1' ) --> creates localhost IPv6 Address {[::1]:0}" )
		local adr = Address( '::1' )
		asrtHlp.Address( adr, "AF_INET6", '::1', 0 ) -- AF_INET6 inferred from IP6 Address
	end,

	test_CreateIp4AddressPort = function( self )
		local family,ip, port  = "AF_INET", Interface( 'default' ).AF_INET.address.ip, 8000
		Test.Case.describe( fmt( "Socket.Address( '%s', %d ) --> creates IPv4 Address {%s:%d}", ip,port,ip,port ) )
		local adr = Address( ip, port)
		asrtHlp.Address( adr, family, ip, port )
	end,

	test_CreateIp6AddressPort = function( self )
		local family,ip, port  = "AF_INET6", Interface( 'default' ).AF_INET6.address.ip, 8000
		Test.Case.describe( fmt( "Socket.Address( '%s', %d ) --> creates IPv4 Address {%s:%d}", ip,port,ip,port ) )
		local adr = Address( ip, port)
		asrtHlp.Address( adr, family, ip, port )
	end,
	
	test_CreateIp4AddressByClone = function( self )
		local family,ip, port  = "AF_INET", Interface( 'default' ).AF_INET.address.ip, 8000
		Test.Case.describe( "Socket.Address( adr ) --> creates clone of IPv4 Address" )
		local adr      = Address( ip, port )
		local adrClone = Address( adr )
		T.assert( adr == adrClone, "`%s` should equal `%s`", adr, adrClone )
		asrtHlp.Address( adr, family, ip, port )
	end,
	
	test_CreateIp6AddressByClone = function( self )
		local family,ip, port  = "AF_INET6", Interface( 'default' ).AF_INET6.address.ip, 8000
		Test.Case.describe( "Socket.Address( adr ) --> creates clone of IPv6 Address" )
		local adr      = Address( ip, port )
		local adrClone = Address( adr )
		T.assert( adr == adrClone, "`%s` should equal `%s`", adr, adrClone )
		asrtHlp.Address( adr, family, ip, port )
	end,

	test_SetFamily = function( self )
		local family4, ip4, port  = "AF_INET",  Interface( 'default' ).AF_INET.address.ip, 8000
		local family6, ip6, port  = "AF_INET6", Interface( 'default' ).AF_INET6.address.ip, 8000
		Test.Case.describe( "adr.family = AF_INET6 --> changes family to IPv6" )
		local adr = Address( ip4, port )
		asrtHlp.Address( adr, family4, ip4, port )
		adr.family = family6
		asrtHlp.Address( adr, family6, '::', port )
	end,

	test_SetIp = function( self )
		local family4, ip4, port  = "AF_INET",  Interface( 'default' ).AF_INET.address.ip, 0
		Test.Case.describe( fmt( "adr.ip = '%s' sets the ip ", ip4 ) )
		local adr = Address( )
		asrtHlp.Address( adr, family4, '0.0.0.0', port )
		adr.ip = ip4
		asrtHlp.Address( adr, family4, ip4, port )
	end,

	test_SetIpChangesFamily = function( self )
		local family4, ip4, port  = "AF_INET",  Interface( 'default' ).AF_INET.address.ip, 8000
		local family6, ip6, port  = "AF_INET6", Interface( 'default' ).AF_INET6.address.ip, 8000
		Test.Case.describe( fmt( "adr.ip = '%s' sets the ip6 and AF_INET6 ", ip6 ) )
		local adr = Address( ip4, port )
		asrtHlp.Address( adr, family4, ip4, port )
		adr.ip = ip6
		asrtHlp.Address( adr, family6, ip6, port )
	end,

	test_SetPort = function( self )
		local family, ip, port = 'AF_INET', '0.0.0.0', 8000
		Test.Case.describe( fmt( "adr.port = %d sets the port", port ) )
		local adr = Address( )
		asrtHlp.Address( adr, family, ip, 0 )
		adr.port = port
		asrtHlp.Address( adr, family, ip, port )
	end,

	test_EqTestForEquality = function( self )
		local ip4, port4  = Interface( 'default' ).AF_INET.address.ip, 50000
		local ip6, port6  = Interface( 'default' ).AF_INET6.address.ip, 8000
		local adr4a = Address( ip4, port4 )
		local adr4b = Address( ip4, port4 )
		Test.Case.describe( fmt( "%s ~= %s", adr4a, adr4b ) )
		T.assert( adr4a == adr4b, "%s should be equal to %s", adr4a, adr4b )
		local adr6a = Address( ip6, port6 )
		local adr6b = Address( ip6, port6 )
		T.assert( adr6a == adr6b, "%s should be equal to %s", adr6a, adr6b )
	end,

	test_EqTestForPortInEquality = function( self )
		local ip4, port4  = Interface( 'default' ).AF_INET.address.ip, 50000
		local ip6, port6  = Interface( 'default' ).AF_INET6.address.ip, 8000
		local adr4a = Address( ip4, port4 )
		local adr4b = Address( ip4, port4+1 )
		Test.Case.describe( fmt( "%s ~= %s", adr4a, adr4b ) )
		T.assert( adr4a ~= adr4b, "%s should not be equal to %s", adr4a, adr4b )
		local adr6a = Address( ip6, port6 )
		local adr6b = Address( ip6, port6+1 )
		T.assert( adr6a ~= adr6b, "%s should not be equal to %s", adr6a, adr6b )
	end,

	test_EqTestForIpInEquality = function( self )
		local ip4, port4  = Interface( 'default' ).AF_INET.address.ip, 50000
		local ip6, port6  = Interface( 'default' ).AF_INET6.address.ip, 8000
		if ip4 == '127.0.0.1' then Test.Case.skip('Insufficient network setup') end
		local adr4a = Address( ip4, port4 )
		local adr4b = Address( '127.0.0.1', port4 )
		Test.Case.describe( fmt( "%s ~= %s", adr4a, adr4b ) )
		T.assert( adr4a ~= adr4b, "%s should not be equal to %s", adr4a, adr4b )
		local adr6a = Address( ip6, port6 )
		local adr6b = Address( '::1', port6 )
		T.assert( adr6a ~= adr6b, "%s should not be equal to %s", adr6a, adr6b )
	end,

	test_EqTestForFamilyInEquality = function( self )
		local ip4, port4  = Interface( 'default' ).AF_INET.address.ip,  8000
		local ip6, port6  = Interface( 'default' ).AF_INET6.address.ip, 8000
		local adrA = Address( ip4, port4 )
		local adrB = Address( ip6, port6 )
		Test.Case.describe( fmt( "%s ~= %s", adrA, adrB ) )
		T.assert( adrA ~= adrB, "%s should not be equal to %s", adrA, adrB )
	end,

	test_TostringFormatsProperly = function( self )
		local ip4, port4  = Interface( 'default' ).AF_INET.address.ip, 1234
		local ip6, port6  = Interface( 'default' ).AF_INET6.address.ip, 4321
		Test.Case.describe( fmt( "Socket.Address( '%s', %d ) --> T.Net.Address{%s:%d}", ip4,port4,ip4,port4 ) )
		local adr4, str4 = Address( ip4, port4 ), fmt( "T.Net.Address{%s:%d}", ip4, port4 )
		local tostr4     = tostring( adr4 ):gsub( "}: .*", "}" )
		T.assert( str4 == tostr4,  "%s shall be %s", tostr4, str4 )
		local adr6, str6 = Address( ip6, port6 ), fmt( "T.Net.Address{[%s]:%d}", ip6, port6 )
		local tostr6     = tostring( adr6 ):gsub( "}: .*", "}" )
		T.assert( str6 == tostr6,  "%s shall be %s", tostr6, str6 )
	end,


}

return Test( tests )
