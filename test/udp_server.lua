#!../out/lua

udpsock = net.Socket('UDP')
--ip      = net.IpEndpoint('192.168.0.123', 8888)
--ip      = net.IpEndpoint('127.0.0.1', 8888)
ip      = net.IpEndpoint(net.IpEndpoint.localhost, 8888)
print(udpsock, ip)
udpsock:bind(ip)
msg, len, ip_cli = udpsock:recvFrom()
print(msg, len, ip_cli, "\n")
