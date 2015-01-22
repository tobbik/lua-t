#!../out/bin/lua
local t=require('t')
local sport=8005

--srv,ip = t.Socket.bind('TCP', sport)
--srv:listen(5)
srv,ip = t.Socket.listen(sport, 5)
print( srv, ip )
x=0

conns = {}      -- hold all receiving sending information

while true do
	local rconns,wconns  = { srv }, { }
	for i,c in pairs( conns ) do
		if c.rcv then table.insert( rconns, c.sck )
		else          table.insert( wconns, c.sck )
		end
	end

	local rds,wrs = t.select( rconns, wconns )
	print( #rds,#wrs, #rconns, #wconns )

	for n,cli in ipairs( rds ) do
		-- new connection
		if cli == srv then
			local s,a =  srv:accept()
			conns[ s:getId() ] = {
				sck = s,
				ip  = a,
				rcv = true
			}
			print('\tCONNECT: ' ..tostring(s).. " FROM:  "..tostring(a) )
		else
			local msg, len = cli:recv()
			print( msg )
			if msg:find('\n\r\n') then
				local cn = conns[ cli:getId() ]
				cn.rcv = false
				cn.payload = 'HTTP/1.1 200 OK\r\r' ..
                         'Content-Length: 17\r\n' ..
                         'Date: Tue, 20 Jan 2015 20:56:55 GMT\r\n\r\n' ..

                         'This is my answer'
			end
		end
	end

	for n,cli in ipairs( wrs ) do
		local cn = conns[ cli:getId() ]
		local len = cn.sck:send( cn.payload )
		print('\tsend : ' ..len.. " BYTES FROM:  "..tostring(cn.sck) )
		conns[ cli:getId() ] = nil
		cn.sck:close()
	end
	x = x+1
end
srv:close()
