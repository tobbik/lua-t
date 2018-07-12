local ptcs = require't.net'.sck.ptc

local setAliases = function( name, value, p )
	local sn = name:match( "_(.+)" )                       -- 'IPPROTO_TCP' -> 'TCP'
	p[ sn ]                                       = value  -- p['TCP'] = 6
	p[ sn:sub(1,1):upper() .. sn:sub(2):lower() ] = value  -- p['Tcp'] = 6
	p[ sn:lower() ]                               = value  -- p['tcp'] = 6
	p[ value ]                                    = name   -- p[ 6 ]   = 'IPPROTO_TCP'
end

local aliases = {}

for prot, val in pairs( ptcs ) do
	setAliases( prot, val, aliases )
end

for alias, val in pairs( aliases ) do
	ptcs[ alias ] = val
end

return ptcs
