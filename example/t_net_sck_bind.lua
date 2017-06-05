#!../out/bin/lua

-- Explore all ways bind() and listen() can be used on a server socket
Socket    = require't.Net.Socket'
Address   = require't.Net.Address'
Interface = require't.Net.Interface'
ipAddr  = Interface( 'default' ).AF_INET.address:get()
port1   = 8001
port2   = 8002
port3   = 8003
port4   = 8004
port5   = 8005
port6   = 8006
port7   = 8007
port8   = 8008

---  All (obvious) permutations of using bind()
-- 1. create all separately then bind()
-- This is most verbose. It allows to create a different type socket than
-- the default one and to assign socket options before using bind().  Also
-- allows to reuse an existing Address.
sTcp1 = Socket( ) -- implicit TCP,ip4
iTcp1 = Address( ipAddr, port1 )
sTcp1:bind( iTcp1 )
print( sTcp1, iTcp1, '\n' )

-- 2. create Socket separately then bind() to values, return Address only
-- This is relatively compact. It allows to create a different type socket than
-- the default one and to assign socket options before using bind()
sTcp2 = Socket( ) -- implicit TCP,ip4
iTcp2 = sTcp2:bind( ipAddr, port2 )
print( sTcp2, iTcp2, '\n' )

-- 3. create Address separately then bind(), creates and returns socket
-- This is useful for using an existing address
iTcp3 = Address( ipAddr, port3 )
sTcp3 = Socket.bind( iTcp3 ) -- implicit TCP,ip4
print( sTcp3, iTcp3, '\n' )

-- 4. create Socket and Address via bind()
-- Most compact, least flexible
sTcp4, iTcp4 = Socket.bind( ipAddr, port4 ) -- implicit TCP,ip4
print( sTcp4, iTcp4, '\n' )


-- listen()
-- 5. create Socket and Address separately then listen() on it with default backlog
-- Most compact, least flexible
--sTcp5, iTcp5 = Socket.listen( ) -- implicit TCP,ip4
--print( sTcp5, iTcp5, '\n' )

-- 6. create Socket and Address separately then listen() on it with default backlog
-- Most compact, least flexible
sTcp6, iTcp6 = Socket.listen( 3 ) -- implicit TCP,ip4
print( sTcp6, iTcp6, '\n' )

-- 7. create Socket and Address separately then listen() on it
-- Most compact, least flexible
sTcp7, iTcp7 = Socket.bind( ipAddr, port7 ) -- implicit TCP,ip4
sTcp7:listen( 3 )
print( sTcp7, iTcp7, '\n' )

--[[
ip=sTcp5:listen       ( sck, host, port, bl )
ip=sTcp5:listen       ( sck, port, bl )
sTcp5:listen          ( sck, ip, bl )
sTcp5:listen          ( sck, bl )
sck,ip = Socket.listen( host, port, bl )
sck,ip = Socket.listen( port, bl )
sck = Socket.listen   ( ip, bl )
sck = Socket.listen   ( ip )
--]]
