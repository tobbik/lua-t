local Net = require"t.net"
local fmt = string.format

local _mt
local _name   = "t.Net.Interface"

_mt = {       -- local _mt at top of file
	-- essentials
	__name     = _name,
	__tostring = function( self )
		local ip,port
		if self.AF_INET and self.AF_INET.address then
			ip,port = self.AF_INET.address.ip, self.AF_INET.address.port
		elseif self.AF_INET6 and self.AF_INET6.address then
			ip,port = self.AF_INET6.address.ip, self.AF_INET6.address.port
		end
		return Net.ifc.tostring( self ):gsub( "}: ", fmt( "(%s)}: ", ip ) )
	end
}

-- this allows the C-code to lua_getmetatable() and assign it
debug.getregistry()[ "T.Net.Interface" ] = _mt

Net.ifc.default = function( )
	local list      = Net.ifc.list( )
	local candidate = list.lo
	for name,ifc in pairs( list ) do
		if name:lower():match( 'docker' ) then goto continue end
		local f = ifc.flags
		if not f then goto continue end
		if not f.IFF_RUNNING or not f.IFF_BROADCAST or not f.IFF_UP or not f.IFF_MULTICAST then
			goto continue
		end
		if not candidate or ifc.stats.rx_bytes > candidate.stats.rx_bytes then
			candidate = ifc
		end
		::continue::
	end
	return candidate and candidate or list.lo -- use loopback as fallback
end

return Net.ifc
