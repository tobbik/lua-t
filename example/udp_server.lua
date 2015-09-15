#!../out/bin/lua
local t=require('t')
ipAddr,port = '10.128.3.145', 8888 

udpsock = t.Net.UDP( )
ip      = t.Net.IPv4( ipAddr, port )
print( udpsock, ip )
udpsock:bind( ip )
msg, len, ip_cli = udpsock:recvfrom()
print( msg, len, ip_cli, "\n" )
