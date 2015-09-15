#!../out/bin/lua
local t=require('t')

local start_wall_clock = t.Time( )
local tcpsock,ip = t.Net.TCP.connect( '128.30.52.37', 80 )
local len     = tcpsock:send( "GET /TR/REC-html32.html HTTP/1.0\r\n\r\n" )
local buffer = {}
local length = 0

-- this select loop makes no sense, but prooves that select is in fact working
-- as expected
while true do
	res = t.Net.select( {tcpsock},{} )
	io.write( tostring( res[1] ) ..'\t'.. tostring( tcpsock ) .. '\t... ' )
	msg, len = res[1]:recv( )
	if len<1 then
		break
	else
		table.insert( buffer, msg )
		length = length + len
		print(len)
	end
	--msg, len = tcpsock:recv()
	--io.write(msg)
end
print( "\nDONE", #( table.concat( buffer ) ), length )
tcpsock:close( )
start_wall_clock:since( )
print( string.format( "Wall Time: %s    - Proc Time: %s",
	start_wall_clock:get( ),
	os.clock( )
	) )

