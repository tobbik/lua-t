local Time,Socket=require't.Time', require't.Net.Socket'

-- use a standard socket read to download an HTTP document
-- select triggeres readability of socket

local start_wall_clock = Time( )
local tcpsock,adr = Socket.connect( '128.30.52.37', 80 )
local len    = tcpsock:send( "GET /TR/REC-html32.html HTTP/1.0\r\n\r\n" )
local buffer = {}
local length = 0

-- this select loop makes no sense for this script, but prooves that select is
-- in fact working as expected
while true do
	local res,_ = Socket.select( {tcpsock},{} )
	msg, len = res[1]:recv( )  -- only one socket is observed, must be the first in res
	if msg then
		table.insert( buffer, msg )
		length = length + len
		print(len)
	else
		break
	end
end
print( "\nDONE", #( table.concat( buffer ) ), length )
tcpsock:close( )
start_wall_clock:since( )
print( string.format( "Wall Time: %s    - Proc Time: %s",
	start_wall_clock:get( ),
	os.clock( )
	) )

