#!../out/bin/lua
Net,fmt=require('t.Net'),string.format
ipAddr,port=Net.Interface( 'default' ).AF_INET.address.ip, 8888

local s    = Net.Socket( 'UDP' ) -- implicit ip4
local ip   = s:bind( ipAddr, port )
print( s, ip )
for k,v in pairs(getmetatable(s)) do print( k, v ) end

-- this select loop makes no sense, but prooves that select is in fact working
-- as expected
while true do
	res = Net.Socket.select({s},{})
	print(#res, s)
	local cIp = Net.Address()
	msg, len= res[1]:recv( ip )
	if len<1 then
		break
	else
		print( msg, len, ip )
		if 'exit' == msg:sub( 1,4 ) then
			break
		end
	end
end
s:close( )

