#!../out/bin/lua
Net,fmt=require('t.Net'),string.format
ipAddr,port=Net.Interface( 'default' ).address:get(),8888

udpsock = Net.Socket( 'UDP', 'ip4' )
ip      = Net.Address( ipAddr, port )
print( udpsock, ip )
--for k,v in pairs(getmetatable(udpsock)) do print( k, v ) end
udpsock:bind( ip )
ip_cli = Net.Socket()
msg, len = udpsock:recv(ip_cli)
print( msg, len, ip_cli, "\n" )
