xt=require'xt'


t = xt.Packer.Struct (
	{ length   = xt.Packer.Int(2) },
	{ ['type'] = xt.Packer.Int(2) },
	-- BitField is a special type as the constructor resets the actual offset for each single bit
	-- boolean flags -> status
	{ status   = xt.Packer.Struct (
	 {isHealthy   = xt.Packer.Bit( 1 )},  -- is the train set healthy?
	 {isZeroSpeed = xt.Packer.Bit( 1 )},  -- is the train set stationary?
	 {isMaintMode = xt.Packer.Bit( 1 )},  -- is the train set in maint mode?
	 {isPassReq   = xt.Packer.Bit( 1 )},  -- is a passenger request active?
	 {isFileForDl = xt.Packer.Bit( 1 )},  -- is a file waiting for download (RTDM ready)
	 {isVmdsConnd = xt.Packer.Bit( 1 )},  -- is the connection to the vmds lost?
	 {isShopMode  = xt.Packer.Bit( 1 )},  -- is the train set in shop mode?
	 {padding     = xt.Packer.Bit( 1 )}   -- padding
	)},
	{['@status']    = xt.Packer.Int(1)},
	{ConsistCount   = xt.Packer.Int(1)}
)
q = xt.Packer.Struct (
	xt.Packer.Int(2),
	xt.Packer.Int(2),
	xt.Packer.Int(1),
	xt.Packer.Int(1)
)
b=xt.Buffer( 'ABCDEFGH' )
--t( b, 0 )

--[[
m=getmetatable(t)

s= {_blah=4,
	_packer =	{ 
	length   = xt.Packer.Int(2),
	['type'] = xt.Packer.Int(2),
	['@status']    = xt.Packer.Int(1),
	ConsistCount   = xt.Packer.Int(1)
}}
table.insert(s._packer, s._packer.length)
table.insert(s._packer, s._packer['type'])
table.insert(s._packer, s._packer['@status'])
table.insert(s._packer, s._packer.ConsistCount)

s=setmetatable(s,m)

status   = xt.Packer.Struct (
	-- boolean flags -> status
	 {isHealthy   = xt.Packer.Bit( 1 )},  -- is the train set healthy?
	 {isZeroSpeed = xt.Packer.Bit( 1 )},  -- is the train set stationary?
	 {isMaintMode = xt.Packer.Bit( 1 )},  -- is the train set in maint mode?
	 {isPassReq   = xt.Packer.Bit( 1 )},  -- is a passenger request active?
	 {isFileForDl = xt.Packer.Bit( 1 )},  -- is a file waiting for download (RTDM ready)
	 {isVmdsConnd = xt.Packer.Bit( 1 )},  -- is the connection to the vmds lost?
	 {isShopMode  = xt.Packer.Bit( 1 )},  -- is the train set in shop mode?
	 {padding     = xt.Packer.Bit( 1 )}   -- padding
)

t1= xt.Packer.Struct (
	{length       = xt.Packer.Int( 2 )},
	{['type']     = xt.Packer.Int( 2 )},
	{status       = status},
	{['@status']  = xt.Packer.Int( 1 )},
	{ConsistCount = xt.Packer.Int( 1 )}
)

t2 =xt.Packer.Struct (
	{length       = xt.Packer.Int( 2 )},
	{['type']     = xt.Packer.Int( 2 )},
	{status       = status},
	{['@status']  = xt.Packer.Int( 1 )},
	{ConsistCount = xt.Packer.Int( 1 )}
)
--]]
