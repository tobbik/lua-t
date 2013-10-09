#!../out/lua

tcpsock = net.createTcp()
ip      = net.createIp('128.30.52.37', 80)
net.connect(tcpsock, ip)
len = net.send(tcpsock,"GET /TR/REC-html32.html HTTP/1.0\r\n\r\n")
buffer = {}
length = 0
while len>0 do
	msg, len = net.recv(tcpsock)
	table.insert(buffer, msg)
	length = length + len
	print(len)
end
print ("DONE", #(table.concat(buffer)), length)
net.close(tcpsock)

