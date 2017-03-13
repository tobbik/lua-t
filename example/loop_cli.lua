#!../out/bin/lua
local t   = require( 't' )
local fmt = string.format

ipadd     = arg[1] or t.Net.IPv4.localhost

-- a client to interact with the loop.lua example
print_help = function( )
	print( [[
		exit      - send exit to server and exit
		help      - print this help
		remove n  - remove timer from server list where n is timer index
		show      - make server list all its loop events]] )
end

s  = t.Net.Socket( 'ip4', 'UDP' )
ip = t.Net.IPv4( ipadd, 8888 )
print( s, ip )
while true do
	io.write( "Enter command and type enter ('help' for command list': " )
	local cmd = io.read( '*l' )
	if 'HELP' == cmd:upper( ) then print_help( ) end
	s:send( ip, cmd )
	if 'EXIT' == cmd:upper( ) then break end
end
s:close()
