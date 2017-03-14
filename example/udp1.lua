#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr,port=t.Net.Interface( 'default' ).address:get(),8888

local s    = t.Net.Socket( 'UDP' ) -- implicit ip4
local s,ip = s:bind( ipAddr, port )
print( s, ip )
for k,v in pairs(getmetatable(s)) do print( k, v ) end

-- this select loop makes no sense, but prooves that select is in fact working
-- as expected
while true do
	res = t.Net.Socket.select({s},{})
	print(#res, s)
	msg, len, ip = res[1]:recv( )
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

