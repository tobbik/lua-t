#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr,port=t.Net.Interface( 'default' ).address:get(),8888

udpsock = t.Net.Socket( 'UDP', 'ip4' )
ip      = t.Net.IPv4( ipAddr, port )
print( udpsock, ip )
--for k,v in pairs(getmetatable(udpsock)) do print( k, v ) end
udpsock:bind( ip )
msg, len, ip_cli = udpsock:recv()
print( msg, len, ip_cli, "\n" )
