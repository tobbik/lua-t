Socket,Address,Interface,Buffer =
	require('t.Net.Socket'),require('t.Net.Address'),require('t.Net.Interface'),require('t.Buffer')
ipAddr = arg[1] and arg[1] or Interface( 'default' ).AF_INET.address.ip
port   = arg[2] and arg[2] or 8888

udpsock = Socket( 'UDP' ) --implicit ip4
adr     = Address( ipAddr, port )
udpsock:send( Buffer("This is my message to you\n"), adr, 15 ) -- send 15 bytes of Buffer to adr
