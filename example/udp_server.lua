#!../out/bin/lua
local t=require('t')

udpsock = t.Socket('UDP')
ip      = t.IpEndpoint('192.168.0.200', 8888)
--ip      = t.IpEndpoint('127.0.0.1', 8888)
--ip      = t.IpEndpoint(t.IpEndpoint.localhost, 8888)
print(udpsock, ip)
udpsock:bind(ip)
msg, len, ip_cli = udpsock:recvFrom()
print(msg, len, ip_cli, "\n")
