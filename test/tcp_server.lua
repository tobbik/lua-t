#!../out/lua

tcpsock = net.createTcp()
--ip      = net.createIP('192.168.0.123', 8888)
ip      = net.createIp('127.0.0.1', 8888)
net.bind(tcpsock,ip)
net.listen(tcpsock,5)
consock = net.accept(tcpsock)
msg, len = net.recv(consock)
print(msg, len, "\n")
net.close(consock)
net.close(tcpsock)
