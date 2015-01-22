#!/usr/bin/lua
local socket=require('socket')

host = "*"
port = 8004
print("Binding to host '" ..host.. "' and port " ..port.. "...")
srv = assert(socket.bind(host, port))
i, p   = srv:getsockname()
assert(i, p)
print("Waiting connection from talker on " .. i .. ":" .. p .. "...")

x=0
payload= 'HTTP/1.1 200 OK\r\r' ..
'Content-Length: 17\r\n' ..
'Date: Tue, 20 Jan 2015 20:56:55 GMT\r\n\r\n' ..
'This is my answer'


while true do
	local cli, a   =  srv:accept()
	local msg, len = cli:receive()
	print('FROM ' ..tostring( cli ).. " RECEIVED:  "..msg )
	while msg~='' do
		--print( len, msg, string.byte(msg) )
		msg, len = cli:receive()
	end
	len = cli:send( payload )
	print('\tsend : ' ..len.. " BYTES VIA:  "..tostring( cli ) .. "["..cli:getfd().."]")
	cli:close()
	--collectgarbage()
	x = x+1
	if 0==x%10 then print("CC");collectgarbage() end
end
srv:close()

local socket = require("socket")
