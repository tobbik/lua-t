#!../out/lua

udpsock = net.createUdp()
--ip      = net.createIP('192.168.0.123', 8888)
ip      = net.createIp('127.0.0.1', 8888)
print(ip)
print(ip:getIp(), ip:getPort())
net.bind(udpsock,ip)
msg, len, ip = net.recvFrom(udpsock)
print(msg, len, ip, "\n")
