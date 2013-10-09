#!../out/lua

tcpsock = net.createTcp()
ip      = net.createIp('127.0.0.1', 8888)
net.connect(tcpsock, ip)
print("send:", net.send (tcpsock, "This is my TCP message to you\n") )
net.close(tcpsock)
