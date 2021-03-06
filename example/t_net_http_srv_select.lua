-- a naive webserver implementation based on the select() system call
-- just to show how it's done, please use t.Http.Server instead

local Socket = require('t.Net.Socket')
local s_port = 8005

srv,adr = Socket.listen( s_port, 5 )
print( srv, adr )
x = 0
remSock = function( t, s)
	for k,v in ipairs(t) do if v==s then table.remove( t, k ) break end end
end

conns = {}      -- hold all receiving sending information
rsocks,wsocks  = { srv = srv }, { }

while true do
	print( #rsocks, #wsocks )
	local rds, wrs = Socket.select( rsocks, wsocks )
	print( #rds, #wrs )
	-- new connection
	if rds.srv then
		local s,a =  rds.srv:accept( )
		table.insert( rsocks, s );
		print('\tCONNECT: ' ..tostring(s).. " FROM:  "..tostring(a) )
	end

	for n,cli in ipairs( rds ) do
		local msg, len = cli:recv( )
		print( msg )
		if msg:find('\n\r\n') then
			conns[ cli ] = {
				payload = 'HTTP/1.1 200 OK\r\r' ..
						 'Content-Length: 17\r\n' ..
						 'Date: Tue, 20 Jan 2015 20:56:55 GMT\r\n\r\n' ..

						 'This is my answer'
			}
			table.insert( wsocks, cli )
			remSock( rsocks, cli )
		end
	end

	for n,cli in ipairs( wrs ) do
		local len = cli:send( conns[ cli ].payload )
		print('\tsend : ' ..len.. " BYTES FROM:  "..tostring(cli) )
		conns[ cli ] = nil
		cli:close( )
		remSock( wsocks, cli )
	end
	x = x+1
end
srv:close()
