#!../out/bin/lua
local xt=require('xt')
local fmt=string.format

udpsock = xt.Socket('UDP')
ip      = xt.IpEndpoint('192.168.0.219', 8888)
-- mip     = xt.IpEndpoint('172.16.1.208')
-- udpsock:bind(mip)
--ip      = xt.IpEndpoint('127.0.0.1', 8888)
--ip      = xt.IpEndpoint('10.128.3.131', 8888)
print (fmt ("%s with ID %d send %d bytes to %s", udpsock, udpsock:getId(), udpsock:sendTo (ip,"This is my message to you\n"), ip))
--udpsock:connect(ip)
--print("send:", udpsock:send("This is my message to you\n") )
