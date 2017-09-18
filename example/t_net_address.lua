write,fmt   = io.write, string.format
Address     = require"t.Net.Address"
Interface   = require"t.Net.Interface"

adr1        = Interface('default').AF_INET.address

print( adr1.ip )

ip_int      = Address.ip2int( adr1.ip )
ip_str      = Address.int2ip( ip_int )

print( ip_int, ip_str )

adr2        = Address( adr1 )   -- copy constructor

print(adr1, adr1.ip)
print(adr2, adr2.ip)

