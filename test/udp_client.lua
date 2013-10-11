#!../out/lua

udpsock = net.Socket.createUdp()
--ip      = net.createIP('192.168.0.123', 8888)
ip      = net.IpEndpoint('127.0.0.1', 8888)
print("send:", udpsock:sendTo(ip,"This is my message to you\n") )
