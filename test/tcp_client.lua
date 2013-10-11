#!../out/lua

tcpsock = net.Socket.createTcp()
ip      = net.IpEndpoint('127.0.0.1', 8888)
tcpsock:connect( ip )
print("send:", tcpsock:send("This is my TCP message to you\n") )
tcpsock:close()
