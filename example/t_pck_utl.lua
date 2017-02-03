local rep = string.rep

local mod
mod = {
	get = function( r, b, rc )
		local rc = rc or 0
		for k,v in pairs(r) do
			io.write( rep( '\t', rc ), k )
			if 'Struct' == Pack.type( v ) then
				print( )
				mod.get( v, b, rc+1 )
			else
				print( '', v, v( b ) )
			end
		end
	end,

	set = function( r, b, vs, rc )
		local rc = rc or 0
		for k,v in pairs(r) do
			io.write( rep( '\t', rc ), k )
			if 'Struct' == Pack.type( v ) then
				print( )
				mod.set( v, b, vs[k], rc+1 )
			else
				v( b, vs[k] )
				print( '', v, v( b ) )
			end
		end
	end
}

return mod
