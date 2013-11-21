#!../out/bin/lua
local xt=require('xt')
local ipadd,port='192.168.0.200',8888

ip      = xt.IpEndpoint(ipadd, port)
mip     = xt.IpEndpoint('172.16.1.208',54321)
tcpsock = xt.Socket('TCP')
tcpsock:bind(mip)
tcpsock:connect(ip)
print(tcpsock, ip, mip)
msg = string.rep('0123456789', 1000010)
sent = 0
print(tcpsock, ip, mip)
while sent<#msg do
	local snt = tcpsock:send(msg, sent)
	print ("SNT:",snt)
	sent = sent + snt
	print(sent, #msg)
end
print("DONE", '\n', sent, "\n")
tcpsock:close()
