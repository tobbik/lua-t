#!../out/bin/lua
local xt=require('xt')


local tcpsock,ip = xt.net.Socket.connect('TCP','128.30.52.37', 80)
local len     = tcpsock:send("GET /TR/REC-html32.html HTTP/1.0\r\n\r\n")
local buffer = {}
local length = 0

-- this select loop makes no sense, but prooves that select is in fact working
-- as expected
while true do
	res = xt.net.select({tcpsock},{})
	print(res[1], tcpsock)
	msg, len = res[1]:recv()
	if len<1 then
		break
	else
		table.insert(buffer, msg)
		length = length + len
		print(len)
	end
	--msg, len = tcpsock:recv()
	--io.write(msg)
end
print ("DONE", #(table.concat(buffer)), length)
tcpsock:close()

