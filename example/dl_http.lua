#!../out/bin/lua
local t=require('t')

local tcpsock,ip = t.Socket.connect('TCP','128.30.52.37', 80)
local len     = tcpsock:send("GET /TR/REC-html32.html HTTP/1.0\r\n\r\n")
local buffer = {}
local length = 0
while len>0 do
	msg, len = tcpsock:recv()
	table.insert(buffer, msg)
	length = length + len
	print(len)
end
print ("DONE", #(table.concat(buffer)), length)
tcpsock:close()
