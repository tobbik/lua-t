local fmls = require't.net'.fml
local map  = require't.Table'.map

for f_name, value in pairs( map( fmls, function( v, k ) return 'string' == type( k ) and v or nil end ) ) do
	local sn = f_name:match( "_(.+)" )                        -- 'AF_INET6' -> 'INET6'
	fmls[ sn ]                                       = value  -- p['INET6'] = 6
	fmls[ sn:sub(1,1):upper() .. sn:sub(2):lower() ] = value  -- p['Inet6'] = 6
	fmls[ sn:lower() ]                               = value  -- p['inet6'] = 6
end

-- define a bunch of convienience aliases
fmls.any          = fmls.AF_UNSPEC

fmls.ip4          = fmls.AF_INET
fmls.Ip4          = fmls.AF_INET
fmls.IP4          = fmls.AF_INET
fmls.IPv4         = fmls.AF_INET

fmls.ip6          = fmls.AF_INET6
fmls.Ip6          = fmls.AF_INET6
fmls.IP6          = fmls.AF_INET6
fmls.IPv6         = fmls.AF_INET6

-- probably not a good idea ..
if not fmls.AF_LINK   then fmls.AF_LINK   = fmls.AF_PACKET end
if not fmls.AF_PACKET then fmls.AF_PACKET = fmls.AF_LINK   end

return fmls
