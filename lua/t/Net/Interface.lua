local Net = require"t.net"
local fmt = string.format

local _mt
local _name   = "t.Net.Interface"

_mt = {       -- local _mt at top of file
	-- essentials
	__name     = _name,
	__tostring = function( self )
		local ip, port
		if self.AF_INET and self.AF_INET.address then
			ip, port = self.AF_INET.address.ip, self.AF_INET.address.port
		elseif self.AF_INET6 and self.AF_INET6.address then
			ip, port = self.AF_INET6.address.ip, self.AF_INET6.address.port
		end
		return Net.ifc.tostring( self ):gsub( "}: ", fmt( "(%s)}: ", ip ) )
	end,
	__index    = function( self, key )
		if 'address' == key then
			-- prefer AF_INET
			if self.AF_INET  and self.AF_INET[ 1 ]  then return self.AF_INET[ 1 ].address  end
			if self.AF_INET6 and self.AF_INET6[ 1 ] then return self.AF_INET6[ 1 ].address end
			return nil
		else
			return rawget( self, key )
		end
	end
}

-- this allows the C-code to lua_getmetatable() and assign it
debug.getregistry( )[ "T.Net.Interface" ] = _mt

Net.ifc.default = function( )
	local list      = Net.ifc.list( )
	local candidate = list.lo -- use loopback as default, overwrite if find something better
	for name,ifc in pairs( list ) do
		local f = ifc.flags
		if not name:lower():match( 'docker' ) and f and f.IFF_RUNNING and f.IFF_BROADCAST and f.IFF_UP and f.IFF_MULTICAST then
			if candidate == list.lo or ifc.stats.rx_bytes > candidate.stats.rx_bytes then
				candidate = ifc
			end
		end
	end
	return candidate
end

return Net.ifc
