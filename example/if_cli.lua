#!../out/bin/lua
local Net,Time=require('t.Net'),require't.Time'
local ip1,ip2,dip,dport= '10.128.3.200','10.128.3.201','10.128.3.144',8888
--local ip1,ip2,dip,dport= '192.168.0.140','192.168.0.131','192.168.0.200',8888

local i=10
tcpsock1, src_ip1 = Net.Socket.bind( ip1 )
tcpsock2, src_ip2 = Net.Socket.bind( ip2 )
dst_ip            = Net.Address( dip,dport )
print( tcpsock1, src_ip1 )
print( tcpsock2, src_ip2 )
print( dst_ip )

tcpsock1:connect( dst_ip )
tcpsock2:connect( dst_ip )
print( tcpsock1, tcpsock2, src_ip1, src_ip2, dst_ip )
while i>0 do
	print( "send:",
		tcpsock1:send( i.." This is my TCP message to you from (1)\n") )
	Time.sleep( 1 )
	print("send:",
		tcpsock2:send( i.." This is my TCP message to you from (2)\n") )
	Time.sleep(1200)
	i = i-1
end
tcpsock1:close( )
tcpsock2:close( )
