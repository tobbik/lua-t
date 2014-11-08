xt=require'xt'


p = xt.Pack.IntB( 2 )
c = xt.Pack.Int( 2 )

ts = xt.Pack.Sequence (
	p,
	xt.Pack.Int( 2 ),
	xt.Pack.Int( 1 ),
	xt.Pack.Int( 1 )
)

ss = xt.Pack.Sequence (
	p,
	p,
	xt.Pack.Sequence (
		xt.Pack.Bit( 0 ),  -- is the train set healthy?
		xt.Pack.Bit( 1 ),  -- is the train set stationary?
		xt.Pack.Bit( 2 ),  -- is the train set in maint mode?
		xt.Pack.Bit( 3 ),  -- is a passenger request active?
		xt.Pack.Bit( 4 ),  -- is a file waiting for download (RTDM ready)
		xt.Pack.Bit( 5 ),  -- is the connection to the vmds lost?
		xt.Pack.Bit( 6 ),  -- is the train set in shop mode?
		xt.Pack.Bit( 7 )   -- padding
	),
	xt.Pack.Int(1),
	xt.Pack.Int(1)
)


s = xt.Pack.Struct (
	{ length       = xt.Pack.Int( 2 ) },
	{ ['type']     = xt.Pack.Int( 2 ) },
 	{ ['@status']  = xt.Pack.Int( 1 ) },
 	{ ConsistCount = xt.Pack.Int( 1 ) }
)

sbits   = xt.Pack.Struct (
	{isHealthy   = xt.Pack.Bit( 0 )},  -- is the train set healthy?
	{isZeroSpeed = xt.Pack.Bit( 1 )},  -- is the train set stationary?
	{isMaintMode = xt.Pack.Bit( 2 )},  -- is the train set in maint mode?
	{isPassReq   = xt.Pack.Bit( 3 )},  -- is a passenger request active?
	{isFileForDl = xt.Pack.Bit( 4 )},  -- is a file waiting for download (RTDM ready)
	{isVmdsConnd = xt.Pack.Bit( 5 )},  -- is the connection to the vmds lost?
	{isShopMode  = xt.Pack.Bit( 6 )},  -- is the train set in shop mode?
	{paddingBit  = xt.Pack.Bit( 7 )}   -- padding
)


t = xt.Pack.Struct (
	{ length    = p },
	{ ['type']  = p },
-- 	-- BitField is a special type as the constructor resets the actual offset for each single bit
-- 	-- boolean flags -> status
	{ status    = sbits},
	{ internal  = ss},
	{ another   = p},
 	{ count     = p},
	{ status2   = sbits},
 	{ count2    = p}
)

s=string.char( 76, 94, 1, 0, 0x55) .. 'abcdefghijklmnop'

b=xt.Buffer( 'ABCDEFGHIJKLMNOPQRS' )

for k,v in pairs(t) do print( k,v ) end




a = xt.Pack.Array( xt.Pack.IntB(2), 4 )
