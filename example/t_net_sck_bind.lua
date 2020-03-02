#!../out/bin/lua

-- Explore all ways bind() and listen() can be used on a server socket
-- connect() works similarily but is not shown here
Socket    = require't.Net.Socket'
Address   = require't.Net.Address'
Interface = require't.Net.Interface'
-- get IP address of most suitable local interface
ipAddr  = Interface.default( ).AF_INET.address.ip
port1   = 8001
port2   = 8002
port3   = 8003
port4   = 8004
port5   = 8005
port6   = 8006
port7   = 8007

---  All (obvious) permutations of using bind()
-- 1. create all separately then bind()
-- This is most verbose. It allows to create a different type socket than
-- the default one and to assign socket options before using bind().  Also
-- allows to reuse an existing Address.
sck = Socket( ) -- implicit TCP,ip4
adr = Address( ipAddr, port1 )
sck:bind( adr )
print( sck, adr )
print( adr, sck:getsockname( ) ) -- adr and getsockname() should have identical
                                 -- values but different userdata instance
print( "Same Address:", adr ==sck:getsockname( ), '\n' )

-- 2. create Socket separately then bind() to values, return Address only
-- This is relatively compact. It allows to create a different type socket than
-- the default one and to assign socket options before using bind()
sck = Socket( ) -- implicit TCP,ip4
adr = sck:bind( ipAddr, port2 )
print( sck, adr )
print( adr, sck:getsockname( ) ) -- adr and getsockname() should have identical
                                 -- values but different userdata instance
print( "Same Address:", adr ==sck:getsockname( ), '\n' )

-- 3. create Address separately then bind(), creates and returns socket
-- This is useful for using an existing address
adr = Address( ipAddr, port3 )
sck = Socket.bind( adr ) -- implicit TCP,ip4
print( sck, adr )
print( adr, sck:getsockname( ) ) -- adr and getsockname() should have identical
                                 -- values but different userdata instance
print( "Same Address:", adr ==sck:getsockname( ), '\n' )

-- 4. create Socket and Address via bind()
-- Most terse, least flexible
sck,adr = Socket.bind( ipAddr, port4 ) -- implicit TCP,ip4
print( sck, adr )
print( adr, sck:getsockname( ) ) -- adr and getsockname() should have identical
                                 -- values but different userdata instance
print( "Same Address:", adr ==sck:getsockname( ), '\n' )
sck:close()

-- Listen is similar to bind
-- 5. create Socket and Address via bind then listen() on it with default backlog
sck,adr = Socket.bind( ipAddr, port5 ) -- implicit TCP,ip5
sck:listen( ) --call listen with one or none argument assumes sck is already bound
print( sck,adr, '\n' )

-- 6. create Socket separeately, perform bind and listen to create address
-- bind() is called implicitely by listen
sck = Socket( ) -- implicit TCP,ip4
adr = sck:listen( ipAddr, port6, 5 ) -- implicit TCP,ip6, backlog 5
print( sck, adr, '\n' )

-- 7. create Socket and Address and listen() all in one command
-- Most compact, least flexible
sck,adr = Socket.listen( ipAddr, port7, 20 ) -- implicit TCP,ip4
print( sck, adr, '\n' )

