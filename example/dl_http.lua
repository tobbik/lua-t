#!../out/bin/lua
local t=require('t')

local start_wall_clock = t.Time( )
local tcpsock,ip = t.Net.TCP.connect( '128.30.52.37', 80 )
print( tcpsock,ip )
local len        = tcpsock:send( "GET /TR/REC-html32.html HTTP/1.0\r\n\r\n" )
local buffer = {}
local length = 0
while len>0 do
	msg, len = tcpsock:recv( )
	table.insert( buffer, msg )
	length = length + len
	print(len)
end
print( "DONE", #(table.concat( buffer )), length )
tcpsock:close( )

start_wall_clock:since( )
print( string.format( "Wall Time: %s    - Proc Time: %s",
	start_wall_clock:get( ),
	os.clock( )
	) )
