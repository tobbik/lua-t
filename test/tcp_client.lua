#!../out/bin/lua
local xt=require('xt')
local ipadd,port='192.168.0.200',8888

ip      = xt.IpEndpoint(ipadd, port)
tcpsock = xt.Socket.connect('TCP', ip)
print(tcpsock, ip)
msg = string.rep('0123456789', 1000010)
sent = 0
while sent<#msg do
	local snt = tcpsock:send(msg, sent)
	print ("SNT:",snt)
	sent = sent + snt
	print(sent, #msg)
end
print("DONE", '\n', sent, "\n")
tcpsock:close()
