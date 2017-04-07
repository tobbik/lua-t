#!out/bin/lua
Net = require't.Net'
-- create a listening server default to IP 0.0.0.0
s,ip1 = Net.Socket.listen( 2345, 5 )
ip2=s:getsockname( )

print( s, ip1, ip2 )
