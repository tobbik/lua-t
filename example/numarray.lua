
local _mt = {
	reverse    = function( p )
		for i=1,math.floor( #p._p/2 ) do
			local t               = p._p[ i ]
			p._p[ i ]             = p._p[ #p._p + 1 - i ]
			p._p[ #p._p + 1 - i ] = t
		end
	end,
	__newindex = function( p, i, v ) rawset( p._p, i, v )          end,
	__len      = function( p )       return #p._p                  end,
	__tostring = function( p )       return "Numbers["..#p._p.."]" end,
}

_mt.count      = _mt.__len
_mt.toString   = _mt.__tostring

_mt.__index    = function( p, i )
	local x = rawget( _mt, i )
	if nil == x then
		return rawget( p._p, i )
	end
	return x
end

local new =  function( )
	return setmetatable( {_p={}}, _mt )
end

return setmetatable( {
		new = new
	},
	{
		__call = new
	} )
