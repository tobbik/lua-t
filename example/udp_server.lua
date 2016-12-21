#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr,port=t.Net.Interface( 'default' ).address:get(),8888

udpsock = t.Net.UDP( )
ip      = t.Net.IPv4( ipAddr, port )
print( udpsock, ip )
--for k,v in pairs(getmetatable(udpsock)) do print( k, v ) end
udpsock:bind( ip )
msg, len, ip_cli = udpsock:recvfrom()
print( msg, len, ip_cli, "\n" )
