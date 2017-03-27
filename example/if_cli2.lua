#!../out/bin/lua
local t=require('t')
local ip1,ip2,dip,dport= '10.128.3.200','10.128.3.201','10.128.3.144',8888
--local ip1,ip2,dip,dport= '192.168.0.140','192.168.0.131','192.168.0.200',8888
local n=10

socks1,socks2={},{}

dst_ip   = t.Net.IPv4( dip,dport )
for i=1,n do
	local sock,ip = t.Net.Socket.bind( ip1 )
	table.insert(socks1, sock)
	sock:connect( dst_ip )
	sock,ip = t.Net.Socket.bind( ip2 )
	table.insert( socks2, sock )
	sock:connect( dst_ip )
end

for k=1,n do
	for i=1,n do
		print( 'send: ', socks1[i]:send( k..' - TCP message from (1:'..i..')\n' ) )
		print( 'send: ', socks2[i]:send( k..' - TCP message from (2:'..i..')\n' ) )
	end
	t.Time.sleep( 100 )
end
