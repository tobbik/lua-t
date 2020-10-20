local Socket            , Address                =
      require"t.net".sck, require"t.net".adr
local Protocol                      , Type                       =
      require"t.Net.Socket.Protocol", require"t.Net.Socket.Type"
local Family               =
      require"t.Net.Family"
local t_type         , t_assert         , type, s_lower     , s_format     , type =
      require't'.type, require't'.assert, type, string.lower, string.format, type
local sck_mt = debug.getregistry( )[ "T.Net.Socket" ]
local Sck_mt = getmetatable( Socket )

local sck_listener    = sck_mt.listener
local sck_binder      = sck_mt.binder
local sck_connecter   = sck_mt.connecter
local sck_shutdowner  = sck_mt.shutdowner
local Socket_new      = Socket.new

-- remove helper functions from socket metatable table
sck_mt.listener       = nil
sck_mt.binder         = nil
sck_mt.connecter      = nil
sck_mt.shutdowner     = nil
Socket.new            = nil

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
		if 'number' == type( port ) then
			return Address( host ), port
		else
			return default, host      -- host is bl
		end
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
		"bad argument #1 to `" ..command.. "` (expected `t.Net.Socket`, got `%s`)", t_type( sck ) )
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
	local sck = Socket( Socket.IPPROTO_TCP, adr.family )
	local t,e = sck:bind( adr )
	--print(sck, adr, t, e )
	if not t then return t, e end -- false, errMsg
	sck_listener( sck, backlog )
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
	sck_listener( sck, backlog )
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
	local sck = Socket( Socket.IPPROTO_TCP, adr.family )
	local t,e = sck_binder( sck, adr )
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
	local t,e = sck_binder( sck, adr )
	--print( sck, adr, t, e )
	if t then return adr else return t,e end
end

Socket.connect = function( host, port )
	local adr = getAddress( host, port )
	local sck = Socket( 'TCP', adr.family )
	local t,e = sck_connecter( sck, adr )
	--print(sck, adr, t, e )
	if t then return sck,adr else return t,e end
end

sck_mt.connect = function( sck, host, port )
	sck = validateSocket( sck, "connect" )
	local adr = getAddress( host, port )
	local t,e = sck_connecter( sck, adr )
	--print( sck, adr, t, e )
	if t then return adr else return t,e end
end

-- lookup shutdown-mode and make sure it's callimg with number
sck_mt.shutdown = function( sck, mode )
	local mode_nr =   "number"==type( mode )
	      and mode
	      or  Socket[ mode ]
	assert( Socket[ mode_nr ], s_format( "Shutdown mode `%s` does not exist.", mode ) )
	sck_shutdowner( sck, mode_nr )
end

-- Socket( ) Creates a new socket
-- \param   protocol string/number: 'TCP', 'udp', 'IPPROTO_IPV6' ...
-- \param   family   string/number: 'ip4', 'AF_INET6', 'raw' ...
-- \param   type     string: 'stream', 'datagram' ...
-- \usage   Net.Socket( )                   -> create TCP IPv4 Socket
--          Net.Socket( 'TCP' )             -> create TCP IPv4 Socket
--          Net.Socket( 'tcp', 'ip4' )      -> create TCP IPv4 Socket
--          Net.Socket( 'Udp', 'ip4' )      -> create UDP IPv4 Socket
--          Net.Socket( 'UDP', 'ip6' )      -> create UDP IPv6 Socket
Sck_mt.__call = function( Sck, protocol, family, typ )
	local p = protocol or Protocol.IPPROTO_TCP       -- sane default
	p       = ( 'string' == type( p ) ) and Protocol[ p ] or p  -- lookup name
	assert( Protocol[ p ] and 'number' == type( p ), s_format( "Can't find protocol `%s`", protocol ))

	local f = family or Family.AF_INET               -- sane default
	f       = ( 'string' == type( f ) ) and Family[ f ] or f  -- lookup name
	assert( Family[ f ] and 'number' == type( f ), s_format( "Can't find family `%s`", family ))

	local t = typ or  ((Protocol.IPPROTO_TCP == p ) and 'SOCK_STREAM')  -- sane default
	              or  ((Protocol.IPPROTO_UDP == p ) and 'SOCK_DGRAM')
	              or  'SOCK_RAW'
	t       = ( 'string' == type( t ) ) and Type[ t ] or t     -- lookup name
	assert( Type[ t ] and 'number' == type( t ), s_format( "Can't find socket type `%s`", typ ))

	return Socket_new( p, f, t )
end

-- set up aliases for the Read/Write shutdown directions
if Socket.SHUT_RD then
	Socket.r,Socket.rd,Socket.read = Socket.SHUT_RD,Socket.SHUT_RD,Socket.SHUT_RD
end

if Socket.SHUT_WR then
	Socket.w,Socket.wr,Socket.write = Socket.SHUT_WR,Socket.SHUT_WR,Socket.SHUT_WR
end

if Socket.SHUT_RDWR then
	Socket.rw, Socket.rdwr, Socket.readwrite = Socket.SHUT_RDWR,Socket.SHUT_RDWR,Socket.SHUT_RDWR
end

return Socket
