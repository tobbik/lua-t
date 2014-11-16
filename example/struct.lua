#!../out/bin/lua
xt=require'xt'


p = xt.Pack.IntB2
c = xt.Pack.Int2

ts = xt.Pack.Sequence (
	p,
	xt.Pack.Int2,
	xt.Pack.Int1,
	xt.Pack.Int1
)

ss = xt.Pack.Sequence (
	p,
	p,
	xt.Pack.Sequence (
		xt.Pack.Bit1,  -- is the train set healthy?
		xt.Pack.Bit2,  -- is the train set stationary?
		xt.Pack.Bit3,  -- is the train set in maint mode?
		xt.Pack.Bit4,  -- is a passenger request active?
		xt.Pack.Bit5,  -- is a file waiting for download (RTDM ready)
		xt.Pack.Bit6,  -- is the connection to the vmds lost?
		xt.Pack.Bit7,  -- is the train set in shop mode?
		xt.Pack.Bit8   -- padding
	),
	xt.Pack.Int1,
	xt.Pack.Int1
)


s = xt.Pack.Struct (
	{ length       = xt.Pack.Int2 },
	{ ['type']     = xt.Pack.Int2 },
	{ ['@status']  = xt.Pack.Int1 },
	{ ConsistCount = xt.Pack.Int1 }
)

sbits   = xt.Pack.Struct (
	{isHealthy   = xt.Pack.Bit1},  -- is the train set healthy?
	{isZeroSpeed = xt.Pack.Bit2},  -- is the train set stationary?
	{isMaintMode = xt.Pack.Bit3},  -- is the train set in maint mode?
	{isPassReq   = xt.Pack.Bit4},  -- is a passenger request active?
	{isFileForDl = xt.Pack.Bit5},  -- is a file waiting for download (RTDM ready)
	{isVmdsConnd = xt.Pack.Bit6},  -- is the connection to the vmds lost?
	{isShopMode  = xt.Pack.Bit7},  -- is the train set in shop mode?
	{paddingBit  = xt.Pack.Bit8}   -- padding
)


t = xt.Pack.Struct (
	{ length    = p },
	{ ['type']  = p },
-- 	-- BitField is a special type as the constructor resets the actual offset for each single bit
-- 	-- boolean flags -> status
	{ status    = sbits},
	{ interna   = ss},
	{ another   = p},
	{ count     = p},
	{ status2   = sbits},
	{ count2    = p}
)

s=string.char( 76, 94, 1, 0, 0x55) .. 'abcdefghijklmnop'

b=xt.Buffer( 'ABCDEFGHIJKLMNOPQRST' )

for k,v in pairs(t) do print( k, v, v(b) ) end
for k,v in pairs(t.status) do print( k, v, v(b) ) end




a = xt.Pack.Array( xt.Pack.IntB2, 4 )
