#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr = arg[1] and arg[1] or t.Net.Interface( 'default' ).address:get()
port   = arg[2] and arg[2] or 8888

udpsock = t.Net.Socket( 'ip4', 'UDP' )
ip      = t.Net.IPv4( ipAddr, port )

-- mip     = t.IpEndpoint( '10.128.3.201' )
-- udpsock:bind( mip )    -- allow specifying the outgoing interface
print (fmt ("%s with ID %d send %d bytes to %s",
            udpsock,
				udpsock:getFd( ),
				udpsock:send( ip, t.Buffer("This is my message to you\n"), 15 ),
				ip )
		)
--udpsock:connect(ip)
--print("send:", udpsock:send("This is my message to you\n") )
