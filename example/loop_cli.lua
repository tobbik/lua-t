#!../out/bin/lua
local Net = require( 't.Net' )
local fmt = string.format

ipadd     = arg[1] or Net.Address.localhost

-- a client to interact with the loop.lua example
print_help = function( )
	print( [[
		exit      - send exit to server and exit
		help      - print this help
		remove n  - remove timer from server list where n is timer index
		show      - make server list all its loop events]] )
end

s  = Net.Socket( 'UDP', 'ip4' )
ip = Net.Address( ipadd, 8888 )
print( s, ip )
while true do
	io.write( "Enter command and type enter ('help' for command list': " )
	local cmd = io.read( '*l' )
	if 'HELP' == cmd:upper( ) then print_help( ) end
	s:send( ip, cmd )
	if 'EXIT' == cmd:upper( ) then break end
end
s:close()
