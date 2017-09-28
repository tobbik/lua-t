#!../out/bin/lua
local Net = require( 't.Net' )
local fmt = string.format

ipadd     = arg[1] or '127.0.0.1'

-- a client to interact with the loop.lua example
print_help = function( )
	print( [[
		exit      - send exit to server and exit
		help      - print this help
		remove n  - remove timer from server list where n is timer index
		show      - make server list all its loop events
		reg       - show table references in registry]] )
end

s   = Net.Socket( 'UDP', 'ip4' )
adr = Net.Address( ipadd, 8888 )
print( s, adr )
while true do
	io.write( "Enter command and type enter ('help' for command list': " )
	local cmd = io.read( '*l' )
	if 'HELP' == cmd:upper( ) then
		print_help( )
	else
		s:send( cmd, adr )
	end
	if 'EXIT' == cmd:upper( ) then break end
end
s:close()
