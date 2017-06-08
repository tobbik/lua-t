#!../out/bin/lua
Net,fmt=require('t.Net'),string.format
ipAddr,port=Net.Interface( 'default' ).AF_INET.address.ip,8888

udpsock = Net.Socket( 'UDP', 'ip4' )
adr     = Net.Address( ipAddr, port )
print( udpsock, adr )
--for k,v in pairs(getmetatable(udpsock)) do print( k, v ) end
udpsock:bind( adr )
ip_cli = Net.Address()
--msg, len = udpsock:recv( ip_cli, 10, 4 )
msg, len = udpsock:recv( 10 )
print( len, msg, ip_cli, "\n" )
