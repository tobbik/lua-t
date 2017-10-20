local Time, Socket = require't.Time', require't.Net.Socket'

-- use a standard socket read to download an HTTP document

local start_wall_clock = Time( )
local tcpsock,adr = Socket.connect( '128.30.52.37', 80 )
print( tcpsock,adr )
local len    = tcpsock:send( "GET /TR/REC-html32.html HTTP/1.0\r\n\r\n" )
local buffer = { }
local length = 0
local msg    = true
while msg do
	msg, len = tcpsock:recv( )
	length   = length + len
	table.insert( buffer, msg )
	print( len )
end
print( "DONE",  #(table.concat( buffer )), length )
tcpsock:close( )

start_wall_clock:since( )
print( string.format( "Wall Time: %s milliseconds    - Proc Time: %s",
	start_wall_clock:get( ),
	os.clock( )
) )
