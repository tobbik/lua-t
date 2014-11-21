#!../out/bin/lua
local t=require('t')
local fmt=string.format

udpsock = t.Socket('UDP')
ip      = t.IpEndpoint('192.168.0.219', 8888)
-- mip     = t.IpEndpoint('172.16.1.208')
-- udpsock:bind(mip)
--ip      = t.IpEndpoint('127.0.0.1', 8888)
--ip      = t.IpEndpoint('10.128.3.131', 8888)
print (fmt ("%s with ID %d send %d bytes to %s", udpsock, udpsock:getId(), udpsock:sendTo (ip,"This is my message to you\n"), ip))
--udpsock:connect(ip)
--print("send:", udpsock:send("This is my message to you\n") )
