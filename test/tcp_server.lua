#!../out/bin/lua
local xt=require('xt')
local ipadd,port='192.168.0.200',8888

--tcpsock = xt.net.Socket('TCP')
--ip      = xt.net.IpEndpoint(ipadd, port)
--tcpsock:bind(ip)
tcpsock,ip = xt.net.Socket.bind('TCP',ipadd,port)
print(tcpsock, ip)
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
