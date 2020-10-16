local ptcs = require't.net'.sck.ptc
local map  = require't.Table'.map

for p_name, value in pairs( map( ptcs, function( v, k ) return 'string' == type( k ) and v or nil end ) ) do
	local sn = p_name:match( "_(.+)" )                        -- 'IPPROTO_TCP' -> 'TCP'
	ptcs[ sn ]                                       = value  -- p['TCP'] = 6
	ptcs[ sn:sub(1,1):upper() .. sn:sub(2):lower() ] = value  -- p['Tcp'] = 6
	ptcs[ sn:lower() ]                               = value  -- p['tcp'] = 6
end

return ptcs

