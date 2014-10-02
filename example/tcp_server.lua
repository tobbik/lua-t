#!../out/bin/lua
local xt,fmt=require('xt'),string.format
local ipadd,port='192.168.0.219',8888

--tcpsock = xt.Socket('TCP')
--ip      = xt.IpEndpoint(ipadd, port)
--tcpsock:bind(ip)
tcpsock,ip = xt.Socket.bind('TCP',ipadd,port)
print(tcpsock, ip)
tcpsock:listen(5)
consock,cip = tcpsock:accept()
length = 0
len    = 2
while len>1 do
	msg, len = consock:recv()
	print (len, length)
	length=length+len
end
print(fmt("DONE  From: %s Length: %d",cip, length))
consock:close()
tcpsock:close()
