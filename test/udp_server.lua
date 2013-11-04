#!../out/bin/lua
local xt=require('xt')

udpsock = xt.net.Socket('UDP')
ip      = xt.net.IpEndpoint('192.168.0.200', 8888)
--ip      = xt.net.IpEndpoint('127.0.0.1', 8888)
--ip      = xt.net.IpEndpoint(xt.net.IpEndpoint.localhost, 8888)
print(udpsock, ip)
udpsock:bind(ip)
msg, len, ip_cli = udpsock:recvFrom()
print(msg, len, ip_cli, "\n")
