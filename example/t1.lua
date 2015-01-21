#!../out/bin/lua
local t=require('t')
local sport=8000

srv,ip = t.Socket.listen(sport, 5)
print( srv, ip )
x=0
payload= 'HTTP/1.1 200 OK\r\r' ..
'Content-Length: 17\r\n' ..
'Date: Tue, 20 Jan 2015 20:56:55 GMT\r\n\r\n' ..
'This is my answer'


while true do
	local cli, a   =  srv:accept()
	local msg, len = cli:recv()
	print('FROM ' ..tostring( cli ).. " RECEIVED:  "..msg )
	while not msg:find('\r\n\r\n') do
		msg, len = cli:recv()
	end
	len = cli:send( payload )
	print('\tsend : ' ..len.. " BYTES VIA:  "..tostring( cli ) )
	t.Socket.showAllFd()
	cli:close()
	collectgarbage()
	x = x+1
	if 0==x%10 then print("CC");collectgarbage() end
end
srv:close()
