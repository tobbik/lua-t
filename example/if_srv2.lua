#!../out/bin/lua
local t=require('t')
--local sip,sport='10.128.3.131',8888
local sip,sport='192.168.0.200',8888

tcpsock,ip = t.Socket.bind('TCP', sip, sport)
tcpsock:listen(5)
print(tcpsock,ip)
a=0

conns  = { master=tcpsock }
while a<100 do
	res = t.selectK(conns, {})
	io.write('LOOP['..#res..']:')
	for n,cli in pairs(res) do
		io.write('  '..tostring(cli)..'')
	end
	print()

	for n,cli in pairs(res) do
		if cli == tcpsock then
			local s,a =  tcpsock:accept()
			conns [ tostring(a) ] = s
			--table.insert(conns, s)
			print('\tCONNECT: ' ..tostring(s).. " FROM:  "..tostring(a) )
		else
			msg, len = cli:recv()
			if len<1 then
				for i,v in pairs(conns) do
					if v == cli then
						conns[i]=nil
						--table.remove(conns, i)
						print('\tCLOSED: '.. tostring(v))
					end
				end
			else
				print('\tRECIEVED['..len..']: '..msg)
			end
		end
	end
	a=a+1
end
tcpsock:close()
