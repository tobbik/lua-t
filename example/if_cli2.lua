#!../out/bin/lua
local xt=require('xt')
--local ip1,ip2,dip,dport= '10.128.3.200','10.128.3.201','10.128.3.131',8888
local ip1,ip2,dip,dport= '192.168.0.140','192.168.0.131','192.168.0.200',8888
local n=10

socks1,socks2={},{}

dst_ip   = xt.IpEndpoint(dip,dport)
for i=1,n do
	local sock,ip = xt.Socket.bind('TCP', ip1)
	table.insert(socks1, sock)
	sock:connect(dst_ip)
	sock,ip = xt.Socket.bind('TCP', ip2)
	table.insert(socks2, sock)
	sock:connect(dst_ip)
end

for k=1,n do
	for i=1,n do
		print('send: ', socks1[i]:send(k..' - TCP message from (1:'..i..')\n') )
		print('send: ', socks2[i]:send(k..' - TCP message from (2:'..i..')\n') )
	end
	--xt.sleep(100)
end
