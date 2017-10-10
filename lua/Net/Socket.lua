local Socket, Address, t_type, t_assert, type  =
      require"t.net".sck, require"t.net".adr,require't'.type,require't'.assert,type
local sck_mt = debug.getregistry( )[ "T.Net.Socket" ]

-- multi function to negotiate arguments
local getAddressArgs = function( host, port, bl, default )
	if "T.Net.Address" == t_type( host ) then
		if "number" == type( port ) then
			return host, port  -- host is adr, port is bl
		else
			return host        -- host is adr, no bl given
		end
	elseif "string" == type( host ) then
		if "number" == type( port ) then
			return Address( host, port ), bl
		else
			return Address( host ), bl
		end
	elseif "number" == type( host ) then
		return default, host      -- host is bl
	else
		return default
	end
end

local getAddress = function( host, port )
	if "T.Net.Address" == t_type( host ) then
		return host
	elseif nil == port and "number" == type( host ) then
		return Address( host )
	elseif "number" ==type( port ) then
		return Address( host, port )
	else
		return Address( )
	end
end


local validateSocket = function( sck, command )
	t_assert( "T.Net.Socket" == t_type( sck ),
		"bad argument #1 to '" ..command.. "' (t.Net.Socket expected, got `%s`)", t_type( sck ) )
	return sck
end

-- sck,adr = Socket.listen(  )               -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
-- sck,adr = Socket.listen( bl )             -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
-- sck,adr = Socket.listen( adr )            -- Sck IPv4(TCP)
-- sck,adr = Socket.listen( adr, bl )        -- Sck IPv4(TCP)
-- sck,adr = Socket.listen( host )           -- Sck IPv4(TCP); Adr host:(0)
-- sck,adr = Socket.listen( host, port )     -- Sck IPv4(TCP); Adr host:port
-- sck,adr = Socket.listen( host, port, bl ) -- Sck IPv4(TCP); Adr host:port
Socket.listen = function( host, port, bl )
	local adr, backlog = getAddressArgs( host, port, bl, Address( ) )
	local sck = Socket( 'TCP', adr.family )
	local t,e = sck:bind( adr )
	--print(sck, adr, t, e )
	if not t then return t, e end -- false, errMsg
	sck:listener( backlog )
	if 0==adr.port then adr.port = sck:getsockname( ).port end
	return sck, adr
end

-- adr = sck:listen( )                -- just listen; assume bound socket
-- adr = sck:listen( bl )             -- just listen; assume bound socket
-- adr = sck:listen( adr )            -- perform bind and listen
-- adr = sck:listen( adr, bl )        -- perform bind and listen
-- adr = sck:listen( host )           -- Adr host:xxxxx
-- adr = sck:listen( host, port )     -- Adr host:port
-- adr = sck:listen( host, port, bl ) -- Adr host:port
sck_mt.listen = function( sck, host, port, bl )
	sck = validateSocket( sck, "listen" )
	local adr, backlog = getAddressArgs( host, port, bl, nil )
	if adr then sck:bind( adr ) else adr = sck:getsockname( ) end
	sck:listener( backlog )
	if 0==adr.port then adr.port = sck:getsockname( ).port end
	return adr and adr or sck:getsockname( )
end

-- sck, adr = Socket.bind()          --> creates TCP IPv4 Socket and 0.0.0.0:0 address
-- sck, adr = Socket.bind(port)      --> creates TCP IPv4 Socket and 0.0.0.0:port address
-- sck, adr = Socket.bind(host,port) --> creates TCP IPv4 Socket and address
-- sck, adr = Socket.bind(address)   --> creates TCP IPv4 Socket but no address
-- sck, adr = Socket.bind(address)   --> returning socket is bound; getsockname()
Socket.bind = function( host, port )
	local adr = getAddress( host, port )
	local sck = Socket( 'TCP', adr.family )
	local t,e = sck:binder( adr )
	--print(sck, adr, t, e )
	if t then return sck,adr else return t,e end
end

-- adr = s:bind()               --> creates a 0.0.0.0:0 address
-- adr = s:bind(port)           --> creates 0.0.0.0:port address
-- adr = s:bind(host,port)      --> creates address
-- adr = s:bind(address)        --> return `address` and does bind
sck_mt.bind = function( sck, host, port )
	sck = validateSocket( sck, "bind" )
	local adr = getAddress( host, port )
	local t,e = sck:binder( adr )
	--print( sck, adr, t, e )
	if t then return adr else return t,e end
end

Socket.connect = function( host, port )
	local adr = getAddress( host, port )
	local sck = Socket( 'TCP', adr.family )
	local t,e = sck:connecter( adr )
	--print(sck, adr, t, e )
	if t then return sck,adr else return t,e end
end

sck_mt.connect = function( sck, host, port )
	sck = validateSocket( sck, "connect" )
	local adr = getAddress( host, port )
	local t,e = sck:connecter( adr )
	--print( sck, adr, t, e )
	if t then return adr else return t,e end
end

return Socket
