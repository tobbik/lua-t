#!../out/bin/lua
local t=require('t')
local ipadd,port='192.168.0.219',8888

--ip      = t.IpEndpoint(ipadd, port)
--mip     = t.IpEndpoint('192.168.0.219',54321)
--tcpsock = t.Socket('TCP')
--tcpsock:bind(mip)
tcpsock, ip =t.Socket.connect('TCP',ipadd,port)
print(tcpsock, ip)
msg = string.rep('0123456789', 1000010)
sent = 0
print(tcpsock, ip)
while sent<#msg do
	local snt = tcpsock:send(msg, sent)
	print ("SNT:",snt)
	sent = sent + snt
	print(sent, #msg)
end
print("DONE", '\n', sent, "\n")
tcpsock:close()
