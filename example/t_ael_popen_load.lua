---
-- \file    example/t_ael_popen_load.lua
-- \detail  simulate a high load system call scenario

local   t = setmetatable( { _x={} }, {
	-- every access to mykey forces metatble operations
	__index = function( self, k )
		if 'number' ~= type( k ) then
			return self._x[ k ]
		else
			return nil
		end
	end,
	__newindex = function( self, k, v )
		if 'number' ~= type( k ) then
			self._x[ k ] = v
		else
			return error( "Can't write to numeric key" )
		end
	end,
} )

t.mykey = 1
for i = 1,9000000 do
	if 0==i%5000000 then
		print( "Intermediate Step:", t.mykey )
	end
	t.mykey = (t.mykey+i) % 256
end

print( "final value: " .. t.mykey )
