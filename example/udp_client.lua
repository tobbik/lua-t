#!../out/bin/lua
local t=require('t')
local fmt=string.format
ipAddr,port = '10.128.3.145', 8888 

udpsock = t.Net.UDP( )
ip      = t.Net.IPv4( ipAddr, port )

-- mip     = t.IpEndpoint( '10.128.3.201' )
-- udpsock:bind( mip )    -- allow specifying the olutgoing interface
print (fmt ("%s with ID %d send %d bytes to %s",
            udpsock, udpsock:getId( ),
				udpsock:sendto( ip, "This is my message to you\n" ), ip ) )
--udpsock:connect(ip)
--print("send:", udpsock:send("This is my message to you\n") )
