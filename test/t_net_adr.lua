---
-- \file    test/t_net_adr.lua
-- \brief   Test the API of Net.Address

local t_require =   require( "t" ).require
local Test      =   require( "t.Test" )
local Socket    =   require( "t.Net.Socket" )
local Address   =   require( "t.Net.Address" )
local Family    =   require( "t.Net.Family" )
local Interface =   require( "t.Net.Interface" )
local chkAdr    = t_require( "assertHelper" ).Adr
local config    = t_require( "t_cfg" )

return {
	beforeAll    = function( self )
		local d   = Interface.default( )
		self.ipv6 = d.AF_INET6 and #d.AF_INET6 > 0
	end,


	-- Tests
	CreateEmptyAddressIp4 = function( self )
		Test.describe( "Socket.Address() --> creates an IPv4 Address {0.0.0.0:0}" )
		local adr = Address( )
		assert( chkAdr( adr, "AF_INET", '0.0.0.0', 0 ) ) -- AF_INET is default
	end,

	CreateEmptyAddressIp6 = function( self )
		Test.describe( "Socket.Address('::') --> creates an IPv6 Address {[::]:0}" )
		local adr = Address( '::' )
		assert( chkAdr( adr, "AF_INET6", '::', 0 ) ) -- AF_INET6 inferred from IP6 Address
	end,

	CreateLocalhostAddressIp4 = function( self )
		Test.describe( "Socket.Address( '127.0.0.1' ) --> creates localhost IPv4 Address {127.0.0.1:0}" )
		local adr = Address( '127.0.0.1' )
		assert( chkAdr( adr, "AF_INET", '127.0.0.1', 0 ) ) -- AF_INET is default
	end,

	CreateLocalhostAddressIp6 = function( self )
		Test.describe( "Socket.Address( '::1' ) --> creates localhost IPv6 Address {[::1]:0}" )
		local adr = Address( '::1' )
		assert( chkAdr( adr, "AF_INET6", '::1', 0 ) ) -- AF_INET6 inferred from IP6 Address
	end,

	CreateIp4AddressPort = function( self )
		local family,ip, port  = "AF_INET", Interface.default( ).AF_INET[1].address.ip, config.nonPrivPort
		Test.describe( ("Socket.Address( '%s', %d ) --> creates IPv4 Address {%s:%d}"):format( ip,port,ip,port ) )
		local adr = Address( ip, port)
		assert( chkAdr( adr, family, ip, port ) )
	end,

	CreateIp6AddressPort = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local family,ip, port  = "AF_INET6", Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		Test.describe( ("Socket.Address( '%s', %d ) --> creates IPv4 Address {%s:%d}"):format( ip,port,ip,port ) )
		local adr = Address( ip, port)
		assert( chkAdr( adr, family, ip, port ) )
	end,
	
	CreateIp4AddressByClone = function( self )
		local family,ip, port  = "AF_INET", Interface.default( ).AF_INET[1].address.ip, config.nonPrivPort
		Test.describe( "Socket.Address( adr ) --> creates clone of IPv4 Address" )
		local adr      = Address( ip, port )
		local adrClone = Address( adr )
		assert( adr == adrClone, ("`%s` should equal `%s`"):format( adr, adrClone ) )
		assert( chkAdr( adr, family, ip, port ) )
	end,

	CreateIp6AddressByClone = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local family,ip, port  = "AF_INET6", Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		Test.describe( "Socket.Address( adr ) --> creates clone of IPv6 Address" )
		local adr      = Address( ip, port )
		local adrClone = Address( adr )
		assert( adr == adrClone, ("`%s` should equal `%s`"):format( adr, adrClone ) )
		assert( chkAdr( adr, family, ip, port ) )
	end,

	SetFamily = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local family4, ip4, port  = "AF_INET",  Interface.default( ).AF_INET[1].address.ip, config.nonPrivPort
		local family6, ip6, port  = "AF_INET6", Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		Test.describe( "adr.family = AF_INET6 --> changes family to IPv6" )
		local adr = Address( ip4, port )
		assert( chkAdr( adr, family4, ip4, port ) )
		adr.family = family6
		assert( chkAdr( adr, family6, '::', port ) )
	end,

	SetFamilyNumber = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local family4, ip4, port  = "AF_INET",  Interface.default( ).AF_INET[1].address.ip, config.nonPrivPort
		local family6, ip6, port  = "AF_INET6", Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		Test.describe( "adr.family = Family.AF_INET6 --> changes family to IPv6" )
		local adr = Address( ip4, port )
		assert( chkAdr( adr, family4, ip4, port ) )
		adr.family = Family.AF_INET6
		assert( 'number' == type( Family.AF_INET6), "`Net.Family.AF_INET6` should be numeric" )
		assert( chkAdr( adr, family6, '::', port ) )
	end,

	SetIp = function( self )
		local family4, ip4, port  = "AF_INET",  Interface.default( ).AF_INET[1].address.ip, 0
		Test.describe( ("adr.ip = '%s' sets the ip "):format( ip4 ) )
		local adr = Address( )
		assert( chkAdr( adr, family4, '0.0.0.0', port ) )
		adr.ip = ip4
		assert( chkAdr( adr, family4, ip4, port ) )
	end,

	SetIpChangesFamily = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local family4, ip4, port  = "AF_INET",  Interface.default( ).AF_INET[1].address.ip, config.nonPrivPort
		local family6, ip6, port  = "AF_INET6", Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		Test.describe( ("adr.ip = '%s' sets the ip6 and AF_INET6 "):format( ip6 ) )
		local adr = Address( ip4, port )
		assert( chkAdr( adr, family4, ip4, port ) )
		adr.ip = ip6
		assert( chkAdr( adr, family6, ip6, port ) )
	end,

	SetPort = function( self )
		local family, ip, port = 'AF_INET', '0.0.0.0', config.nonPrivPort
		Test.describe( ("adr.port = %d sets the port"):format( port ) )
		local adr = Address( )
		assert( chkAdr( adr, family, ip, 0 ) )
		adr.port = port
		assert( chkAdr( adr, family, ip, port ) )
	end,

	EqTestForIpv4Equality = function( self )
		local ip4, port4  = Interface.default( ).AF_INET[1].address.ip, config.nonPrivPortAlt
		local adr4a = Address( ip4, port4 )
		local adr4b = Address( ip4, port4 )
		Test.describe( ("%s ~= %s"):format( adr4a, adr4b ) )
		assert( adr4a == adr4b, ("%s should be equal to %s"):format( adr4a, adr4b ) )
	end,

	EqTestForIpv6Equality = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local ip6, port6  = Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		local adr6a = Address( ip6, port6 )
		local adr6b = Address( ip6, port6 )
		Test.describe( ("%s ~= %s"):format( adr6a, adr6b ) )
		assert( adr6a == adr6b, ("%s should be equal to %s"):format( adr6a, adr6b ) )
	end,

	EqTestForPortIpv4Inequality = function( self )
		local ip4, port4  = Interface.default( ).AF_INET[1].address.ip, config.nonPrivPortAlt
		local adr4a = Address( ip4, port4 )
		local adr4b = Address( ip4, port4+1 )
		Test.describe( ("%s ~= %s"):format( adr4a, adr4b ) )
		assert( adr4a ~= adr4b, ("%s should not be equal to %s"):format( adr4a, adr4b ) )
	end,

	EqTestForPortIpv6Inequality = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local ip6, port6  = Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		local adr6a = Address( ip6, port6 )
		local adr6b = Address( ip6, port6+1 )
		Test.describe( ("%s ~= %s"):format( adr6a, adr6b ) )
		assert( adr6a ~= adr6b, ("%s should not be equal to %s"):format( adr6a, adr6b ) )
	end,

	EqTestForIpv4Inequality = function( self )
		local ip4, port4  = Interface.default( ).AF_INET[1].address.ip, config.nonPrivPortAlt
		if ip4 == '127.0.0.1' then Test.skip('Insufficient network setup') end
		local adr4a = Address( ip4, port4 )
		local adr4b = Address( '127.0.0.1', port4 )
		Test.describe( ("%s ~= %s"):format( adr4a, adr4b ) )
		assert( adr4a ~= adr4b, ("%s should not be equal to %s"):format( adr4a, adr4b ) )
	end,

	EqTestForIpv6Inequality = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local ip6, port6  = Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		if ip6 == '::1' then Test.skip('Insufficient network setup') end
		local adr6a = Address( ip6, port6 )
		local adr6b = Address( '::1', port6 )
		Test.describe( ("%s ~= %s"):format( adr6a, adr6b ) )
		assert( adr6a ~= adr6b, ("%s should not be equal to %s"):format( adr6a, adr6b ) )
	end,

	EqTestForFamilyInEquality = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local ip4, port4  = Interface.default( ).AF_INET[1].address.ip,  config.nonPrivPort
		local ip6, port6  = Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		Test.describe( ("Socket.Address( '%s', %d ) ~= Socket.Address( '%s', %d )"):format( ip4,port4,ip6,port6 ) )
		local adrA = Address( ip4, port4 )
		local adrB = Address( ip6, port6 )
		Test.describe( ("%s ~= %s"):format( adrA, adrB ) )
		assert( adrA ~= adrB, ("%s should not be equal to %s"):format( adrA, adrB ) )
	end,

	TostringFormatsProperly = function( self )
		local ip4, port4  = Interface.default( ).AF_INET[1].address.ip, config.nonPrivPort
		Test.describe( ("Socket.Address( '%s', %d ) --> T.Net.Address{%s:%d}"):format( ip4,port4,ip4,port4 ) )
		local adr4, str4 = Address( ip4, port4 ), ("T.Net.Address{%s:%d}"):format( ip4, port4 )
		local tostr4     = tostring( adr4 ):gsub( "}: .*", "}" )
		assert( str4 == tostr4,  ("%s shall be %s"):format( tostr4, str4 ) )
	end,

	TostringFormatsIpv6Properly = function( self )
		if not self.ipv6 then Test.skip( 'Test requires IPv6 to be enabled' ) end
		local ip6, port6  = Interface.default( ).AF_INET6[1].address.ip, config.nonPrivPort
		Test.describe( ("Socket.Address( '%s', %d ) --> T.Net.Address{%s:%d}"):format( ip6,port6,ip6,port6 ) )
		local adr6, str6 = Address( ip6, port6 ), ("T.Net.Address{[%s]:%d}"):format( ip6, port6 )
		local tostr6     = tostring( adr6 ):gsub( "}: .*", "}" )
		assert( str6 == tostr6,  ("%s shall be %s"):format( tostr6, str6 ) )
	end,

}

