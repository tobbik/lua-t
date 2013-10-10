#!../out/lua

udpsock = net.createUdp()
--ip      = net.IpEndpoint('192.168.0.123', 8888)
--ip      = net.IpEndpoint('127.0.0.1', 8888)
ip      = net.IpEndpoint(net.IpEndpoint.localhost, 8888)
print(ip)
print(ip:getIp(), ip:getPort())
net.bind(udpsock,ip)
msg, len, ip_cli = net.recvFrom(udpsock)
print(msg, len, ip_cli, "\n")
