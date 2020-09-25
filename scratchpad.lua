--[[
local Loop     = require't.Loop'
local Server   = require't.Http.Server'
local fmt      = string.format

local httpServer = Server( Loop(), print )

local host,port = '0.0.0.0',8000

local srv, adr   = httpServer:listen( host, port, 1000 )
print( fmt( "Started `%s` at `%s` (%s)", srv, adr, srv.family ) )
httpServer.ael:run( )
--]]

Csv     = require't.Csv'
c       = Csv('x.tsv')

--for k,v in pairs(c) do print(k,v) end
--for k,v in pairs(getmetatable(c)) do print(k,v) end\
r = 1
for row in c:rows() do
	print("ROW:",row, #row)
	for i,v in pairs(row) do
		print( r, i, v)
	end
	r = r + 1
end
