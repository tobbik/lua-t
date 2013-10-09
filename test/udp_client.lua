#!../out/lua

udpsock = net.createUdp()
--ip      = net.createIP('192.168.0.123', 8888)
ip      = net.createIp('127.0.0.1', 8888)
print("send:", net.sendTo(udpsock,ip,"This is my message to you\n") )
