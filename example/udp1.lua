#!../out/bin/lua
local xt=require('xt')


local s,ip = xt.Socket.bind('UDP','192.168.0.219', 8888)
print(s,ip)

-- this select loop makes no sense, but prooves that select is in fact working
-- as expected
--[[
while true do
	res = xt.select({s},{})
	print(#res, s)
	msg, len, ip = res[1]:recvFrom()
	if len<1 then
		break
	else
		print(msg, len, ip)
	end
	--msg, len = tcpsock:recv()
	--io.write(msg)
end
--]]
msg, len, ip = s:recvFrom()
s:close()

