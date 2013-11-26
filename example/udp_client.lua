#!../out/bin/lua
local xt=require('xt')

udpsock = xt.Socket.createUdp()
mip     = xt.IpEndpoint('172.16.1.208')
--udpsock:bind(mip)
ip      = xt.IpEndpoint('192.168.0.200', 8888)
--ip      = xt.IpEndpoint('127.0.0.1', 8888)
--ip      = xt.IpEndpoint('10.128.3.131', 8888)
print("sendTo:", udpsock:sendTo(ip,"This is my message to you\n") )
--udpsock:connect(ip)
--print("send:", udpsock:send("This is my message to you\n") )
