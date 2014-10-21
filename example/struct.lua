xt=require'xt'


p = xt.Packer.IntB( 2 )
c = xt.Packer.Int( 2 )

ts = xt.Packer.Struct (
	p,
	xt.Packer.Int( 2 ),
	xt.Packer.Int( 1 ),
	xt.Packer.Int( 1 )
)

ss = xt.Packer.Struct (
	p,
	xt.Packer.Int( 2 ),
	xt.Packer.Struct (
		xt.Packer.Bit( 1 ),  -- is the train set healthy?
		xt.Packer.Bit( 1 ),  -- is the train set stationary?
		xt.Packer.Bit( 1 ),  -- is the train set in maint mode?
		xt.Packer.Bit( 1 ),  -- is a passenger request active?
		xt.Packer.Bit( 1 ),  -- is a file waiting for download (RTDM ready)
		xt.Packer.Bit( 1 ),  -- is the connection to the vmds lost?
		xt.Packer.Bit( 1 ),  -- is the train set in shop mode?
		xt.Packer.Bit( 1 )   -- padding
	),
	xt.Packer.Int(1),
	xt.Packer.Int(1)
)


s = xt.Packer.Struct (
	{ length       = xt.Packer.Int( 2 ) },
	{ ['type']     = xt.Packer.Int( 2 ) },
 	{ ['@status']  = xt.Packer.Int( 1 ) },
 	{ ConsistCount = xt.Packer.Int( 1 ) }
)


t = xt.Packer.Struct (
	{ length   = xt.Packer.Int( 2 ) },
	{ ['type'] = xt.Packer.Int( 2 ) },
-- 	-- BitField is a special type as the constructor resets the actual offset for each single bit
-- 	-- boolean flags -> status
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
 	{['@status']    = xt.Packer.Int( 1 )},
	xt.Packer.Int( 2 ),
 	{ConsistCount   = xt.Packer.Int( 1 )}
)

b=xt.Buffer( 'ABCDEFGH' )
ts( b )
ss( b )
t( b )
s( b )

for k,v in pairs(s) do print( k,v) end
