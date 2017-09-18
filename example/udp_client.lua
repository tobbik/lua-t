#!../out/bin/lua
Net,Buffer,fmt=require('t.Net'),require('t.Buffer'),string.format
ipAddr = arg[1] and arg[1] or Net.Interface( 'default' ).AF_INET.address.ip
port   = arg[2] and arg[2] or 8888

udpsock = Net.Socket( 'UDP' ) --implicit ip4
adr     = Net.Address( ipAddr, port )

-- mip     = Net.Address( '10.128.3.201' )
-- udpsock:bind( mip )    -- allow specifying the outgoing interface
print (fmt ("%s with ID %d send %d bytes to %s",
            udpsock,
				udpsock:getFd( ),
				udpsock:send( adr, Buffer("This is my message to you\n"), 15 ),
				adr )
		)
--udpsock:connect(adr)
--print("send:", udpsock:send("This is my message to you\n") )
