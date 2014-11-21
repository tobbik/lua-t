#!../out/bin/lua
local t=require('t')
t = t.Timer(2000)

i=0
while true do
	x = t.debug.select({}, {}, t)
	t:set(5000) -- Linux resets timeval on select
	print(i, x, t)
	i=i+1
end
