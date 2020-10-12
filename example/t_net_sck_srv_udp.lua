Socket,Address,Interface,Buffer =
	require('t.Net.Socket'),require('t.Net.Address'),require('t.Net.Interface'),require('t.Buffer')
ipAddr = arg[1] and arg[1] or Interface.default( ).address.ip
port   = arg[2] and arg[2] or 8888

udpsock = Socket( 'UDP', 'ip4' )
adr     = Address( ipAddr, port )
print( udpsock, adr )
udpsock:bind( adr )
ip_cli = Address()
msg, len = udpsock:recv( ip_cli, 10 )
print( len, msg, ip_cli, "\n" )
