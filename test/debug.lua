#!../out/bin/lua
local xt=require('xt')
t = xt.net.Timer(2000)

i=0
while true do
	x = xt.debug.select({}, {}, t)
	t:set(5000) -- Linux resets timeval on select
	print(i, x, t)
	i=i+1
end
