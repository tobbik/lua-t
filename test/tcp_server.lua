#!../out/lua

tcpsock = net.Socket('TCP')
ip      = net.IpEndpoint('10.128.3.131', 8888)
tcpsock:bind(ip)
tcpsock:listen(5)
consock = tcpsock:accept()
length = 0
len    = 2
while len>1 do
	msg, len = consock:recv()
	print (len, length)
	length=length+len
end
print("DONE: ", length, "\n")
consock:close()
tcpsock:close()
