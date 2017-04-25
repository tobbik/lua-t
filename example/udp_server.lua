#!../out/bin/lua
Net,fmt=require('t.Net'),string.format
ipAddr,port=Net.Interface( 'default' ).address:get(),8888

udpsock = Net.Socket( 'UDP', 'ip4' )
ip      = Net.Address( ipAddr, port )
print( udpsock, ip )
--for k,v in pairs(getmetatable(udpsock)) do print( k, v ) end
udpsock:bind( ip )
ip_cli = Net.Address()
--msg, len = udpsock:recv( ip_cli, 10, 4 )
msg, len = udpsock:recv( 10 )
print( len, msg, ip_cli, "\n" )
