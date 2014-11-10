#!../out/bin/lua
local xt=require('xt')
local fmt=string.format

-- a client to interact with the loop.lua example
printhelp = function()
	print([[
		exit      - send exit to server and exit
		help      - print this help
		remove n  - remove timer from server list where n is timer index
		show      - make ser list all its loop events]] )
end

s  = xt.Socket('UDP')
ip = xt.Ip( xt.Ip.localhost, 8888 )
print( s, ip )
while true do
	io.write( "Enter command and type enter ('help' for command list': ")
	local cmd = io.read( '*l' )
	if 'HELP' == cmd:upper() then printhelp() end
	s:sendTo( ip, cmd )
	if 'EXIT' == cmd:upper() then break end
end
s:close()
