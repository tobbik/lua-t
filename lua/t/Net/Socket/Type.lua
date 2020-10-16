local typs = require't.net'.sck.typ
local map  = require't.Table'.map

for p_name, value in pairs( map( typs, function( v, k ) return 'string' == type( k ) and v or nil end ) ) do
	local sn = p_name:match( "_(.+)" )                        -- 'SOCK_STREAM' -> 'STREAM'
	typs[ sn ]                                       = value  -- p['STREAM'] = 1
	typs[ sn:sub(1,1):upper() .. sn:sub(2):lower() ] = value  -- p['Stream'] = 1
	typs[ sn:lower() ]                               = value  -- p['stream'] = 1
end

return typs

