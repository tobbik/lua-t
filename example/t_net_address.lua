write,fmt   = io.write, string.format
Address     = require"t.Net.Address"
Interface   = require"t.Net.Interface"

ip          = Interface('default').AF_INET.address

print( ip:get( ) )

ip_int      = Address.ip2int( ip:get() )
ip_str      = Address.int2ip( ip_int )

print( ip_int, ip_str )

ip2         = Address( ip )   -- copy constructor
ip1         = Address( ip:get( ) )

print(ip)
print(ip1)
print(ip2)

