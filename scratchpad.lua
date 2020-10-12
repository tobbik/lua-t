--[[
local Loop     = require't.Loop'
local Server   = require't.Http.Server'
local fmt      = string.format

local httpServer = Server( Loop(), print )

local host,port = '0.0.0.0',8000

local srv, adr   = httpServer:listen( host, port, 1000 )
print( fmt( "Started `%s` at `%s` (%s)", srv, adr, srv.family ) )
httpServer.ael:run( )

Csv     = require't.Csv'
--csv     = Csv( 'sample.tsv', '\t' )
csv     = Csv( 'x.tsv', '\t' )

print( csv )
print( csv.delimiter )
print( csv.quotchar )
print( csv.escapechar )
print( csv.doublequoted )
print( csv.state )
print( csv.handle )

for k,v in pairs(getmetatable(csv)) do print(k,v) end
r = 1
for row in csv:rows( ) do
--for row in csv:lines( ) do
	print( "ROW:", row, #row )
	for i,v in pairs( row ) do
		print( r, i, v)
	end
	r = r + 1
end
pp = require't.Table'.pprint
Interface=require't.Net.Interface'

ifs=Interface.list( )
pp( ifs )

--]]

r=require't'.require
I=require't.Net.Interface'
pp=require't.Table'.pprint
t=r'test/t_net_ifc'

