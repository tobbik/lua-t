#!../out/bin/lua
local xt=require('xt')

udpsock = xt.net.Socket.createUdp()
--ip      = xt.net.IpEndpoint('192.168.0.123', 8888)
ip      = xt.net.IpEndpoint('127.0.0.1', 8888)
print("send:", udpsock:sendTo(ip,"This is my message to you\n") )
