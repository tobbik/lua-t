#!../out/lua

tcpsock = net.Socket.createTcp()
ip      = net.IpEndpoint('127.0.0.1', 8888)
tcpsock:bind(ip)
tcpsock:listen(5)
consock = tcpsock:accept() -- blocking here
msg, len = consock:recv()
print(msg, len, "\n")
consock:close()
tcpsock:close()
