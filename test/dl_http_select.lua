#!../out/lua

tcpsock = net.createTcp()
ip      = net.createIp('128.30.52.37', 80)
net.connect(tcpsock, ip)
len = net.send(tcpsock,"GET /TR/REC-html32.html HTTP/1.0\r\n\r\n")

buffer = {}
length = 0
-- this select loop makes no sense, but prooves that select is in fact working
-- as expected
while true do
	res = net.select({tcpsock},{})
	print(res[1], tcpsock)
	msg, len = net.recv(res[1])
	if len<1 then
		break
	else
		table.insert(buffer, msg)
		length = length + len
		print(len)
	end
	--msg, len = net.recv(tcpsock)
	-- io.write(msg)
end
print ("DONE", #(table.concat(buffer)), length)
net.close(tcpsock)

