#!../out/bin/lua
local xt=require('xt')

tcpsock = xt.net.Socket('TCP')
ip      = xt.net.IpEndpoint('10.128.3.131', 8888)
tcpsock:connect(ip)
msg = string.rep('0123456789', 1000010)
sent = 0
while sent<#msg do
	local snt = tcpsock:send(msg, sent)
	print ("SNT:",snt)
	sent = sent + snt
	print(sent, #msg)
end
print("DONE", '\n', length, "\n")
tcpsock:close()
