#!../out/bin/lua
t=require't'


p = t.Pack.IntB2
c = t.Pack.Int2

ts = t.Pack.Sequence (
	p,
	t.Pack.Int2,
	t.Pack.Int1,
	t.Pack.Int1
)

ss = t.Pack.Sequence (
	p,
	p,
	t.Pack.Sequence (
		t.Pack.Bit1,  -- is the train set healthy?
		t.Pack.Bit2,  -- is the train set stationary?
		t.Pack.Bit3,  -- is the train set in maint mode?
		t.Pack.Bit4,  -- is a passenger request active?
		t.Pack.Bit5,  -- is a file waiting for download (RTDM ready)
		t.Pack.Bit6,  -- is the connection to the vmds lost?
		t.Pack.Bit7,  -- is the train set in shop mode?
		t.Pack.Bit8   -- padding
	),
	t.Pack.Int1,
	t.Pack.Int1
)


s = t.Pack.Struct (
	{ length       = t.Pack.Int2 },
	{ ['type']     = t.Pack.Int2 },
	{ ['@status']  = t.Pack.Int1 },
	{ ConsistCount = t.Pack.Int1 }
)
sN= t.Pack('i2i2i1i1')

sbits   = t.Pack.Struct (
	{isHealthy   = t.Pack.Bit1},  -- is the train set healthy?
	{isZeroSpeed = t.Pack.Bit2},  -- is the train set stationary?
	{isMaintMode = t.Pack.Bit3},  -- is the train set in maint mode?
	{isPassReq   = t.Pack.Bit4},  -- is a passenger request active?
	{isFileForDl = t.Pack.Bit5},  -- is a file waiting for download (RTDM ready)
	{isVmdsConnd = t.Pack.Bit6},  -- is the connection to the vmds lost?
	{isShopMode  = t.Pack.Bit7},  -- is the train set in shop mode?
	{paddingBit  = t.Pack.Bit8}   -- padding
)


tr = t.Pack.Struct (
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

b=t.Buffer( 'ABCDEFGHIJKLMNOPQRST' )

for k,v in pairs(tr) do print( k, v, v(b) ) end
for k,v in pairs(tr.status) do print( k, v, v(b) ) end




a = t.Pack.Array( t.Pack.IntB2, 4 )
