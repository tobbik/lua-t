#!../out/bin/lua
local xt=require('xt')

udpsock = xt.Socket('UDP')
ip      = xt.IpEndpoint('192.168.0.200', 8888)
--ip      = xt.IpEndpoint('127.0.0.1', 8888)
--ip      = xt.IpEndpoint(xt.IpEndpoint.localhost, 8888)
print(udpsock, ip)
udpsock:bind(ip)
msg, len, ip_cli = udpsock:recvFrom()
print(msg, len, ip_cli, "\n")
