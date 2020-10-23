--[[
local Loop     = require't.Loop'
local Server   = require't.Http.Server'
local fmt      = string.format

local httpServer = Server( Loop(), print )

local host,port = '0.0.0.0',8000

local srv, adr   = httpServer:listen( host, port, 1000 )
print( fmt( "Started `%s` at `%s` (%s)", srv, adr, srv.family ) )
httpServer.ael:run( )
--]

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
--]]
r=require't'.require
t=r'test/t_net_sck_bind'
t()

S=require't.Net.Socket'
s,a=S.bind('192.168.17.197',4000)
--s:send('foobgar',a)
--
f,e=io.open("foobar")
--f,e=io.open({})
B64  = require't.Encode.Base64'
src  = "This is my Message to you"
enc  = B64.encode( src )
dec  = B64.decode( enc )
print( '\n', src, '\n', enc, '\n', dec )


RC4 = require't.Encode.Rc4'
r   = RC4('thekey')
r1  = RC4('thekey')
src = "This is my Message to you"
enc = r:crypt( src )
dec = r1:crypt( enc )
print( '\n', src, '\n', enc, '\n', dec )
