write,fmt   = io.write, string.format
IPv4        = require"t".Net.IPv4
If          = require"t".Net.Interface

ip          = If.list( ).default.address

print( ip:get( ) )

ip_int      = IPv4.ip2int( ip:get() )
ip_str      = IPv4.int2ip( ip_int )

print( ip_int, ip_str )

ip2         = IPv4( ip )   -- copy constructor
ip1         = IPv4( ip:get( ) )

print(ip)
print(ip1)
print(ip2)

