#!../out/bin/lua
local xt=require('xt')

local i=10
tcpsock1 = xt.net.Socket('TCP')
tcpsock2 = xt.net.Socket('TCP')
src_ip1  = xt.net.IpEndpoint('10.128.3.200')
src_ip2  = xt.net.IpEndpoint('10.128.3.201')
dst_ip   = xt.net.IpEndpoint('10.128.3.131', 8888)

tcpsock1:bind(src_ip1)
tcpsock2:bind(src_ip2)
tcpsock1:connect(dst_ip)
tcpsock2:connect(dst_ip)
print(tcpsock1, tcpsock2, src_ip1, src_ip2, dst_ip)
while i>0 do
	print("send:", tcpsock1:send(
	i.." This is my TCP message to you from (1)\n") )
	xt.net.sleep(600)
	print("send:", tcpsock2:send(
	i.." This is my TCP message to you from (2)\n") )
	xt.net.sleep(400)
	i=i-1
end
tcpsock1:close()
tcpsock2:close()