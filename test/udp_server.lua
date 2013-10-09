#!../out/lua

udpsock = net.createUdp()
--ip      = net.createIP('192.168.0.123', 8888)
ip      = net.createIp('127.0.0.1', 8888)
net.bind(udpsock,ip)
msg, len, ip, port = net.recvFrom(udpsock)
print(msg, len, ip, port, "\n")
