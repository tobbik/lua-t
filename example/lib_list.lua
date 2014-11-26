#!../out/bin/lua
t=require('t')

local print_table = function(name, t)
	print('ELEMS of ' .. name)
	for k,v in pairs(t) do
		--print(string.format('\t %s\t\t[%s]'), v ,k)
		print('\t\t', type(v), name..k , '\t\t' ,v)
	end
end

local iter    -- declare local
iter = function (prefix, t) -- ...then define (for recursion)
	local st    = {}
	local vals  = {}
	for k,v in pairs( t ) do
		if 'loaded' == k or '_G' ==k then -- exclude recursion
			vals[k] = v
		elseif 'table' == type (v) and '_G' ~= k then
			st[k] = v
		else
			vals[k] = v
		end
	end
	print_table(prefix..'.', vals)
	for k,v in pairs(st) do
		iter(prefix..'.'..k, v)
	end
end

iter('_G', _G)
F=t.Pack('<I6')
f=t.Pack('>i6')
s=t.Pack('<c50')

