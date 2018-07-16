local fmls = require't.net'.fml

local setAliases = function( name, value, p )
	local sn = name:match( "_(.+)" )                       -- 'AF_INET6' -> 'INET6'
	p[ sn ]                                       = value  -- p['INET6'] = 6
	p[ sn:sub(1,1):upper() .. sn:sub(2):lower() ] = value  -- p['Inet6'] = 6
	p[ sn:lower() ]                               = value  -- p['inet6'] = 6
	p[ value ]                                    = name   -- p[ 6 ]     = 'AF_INET6'
end

local aliases = { }

for family, val in pairs( fmls ) do
	setAliases( family, val, aliases )
end

for alias, val in pairs( aliases ) do
	fmls[ alias ] = val
end

-- define a bunch of convienience aliases
fmls.any          = fmls.AF_UNSPEC

fmls.unix         = fmls.AF_UNIX

fmls.ip4          = fmls.AF_INET
fmls.Ip4          = fmls.AF_INET
fmls.IP4          = fmls.AF_INET
fmls.IPv4         = fmls.AF_INET

fmls.ip6          = fmls.AF_INET6
fmls.Ip6          = fmls.AF_INET6
fmls.IP6          = fmls.AF_INET6
fmls.IPv6         = fmls.AF_INET6

return fmls
