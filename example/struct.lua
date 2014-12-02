#!../out/bin/lua
t=require't'


p = t.Pack('<i2')
c = t.Pack('>i2')

ts = t.Pack(
	p,
	t.Pack('>i2'),
	t.Pack('i1'),
	t.Pack('i1')
)

ss = t.Pack (
	p,
	p,
	'rrrrrrrr',
	--t.Pack.Sequence (
	--	t.Pack.Bit1,  -- is the train set healthy?
	--	t.Pack.Bit2,  -- is the train set stationary?
	--	t.Pack.Bit3,  -- is the train set in maint mode?
	--	t.Pack.Bit4,  -- is a passenger request active?
	--	t.Pack.Bit5,  -- is a file waiting for download (RTDM ready)
	--	t.Pack.Bit6,  -- is the connection to the vmds lost?
	--	t.Pack.Bit7,  -- is the train set in shop mode?
	--	t.Pack.Bit8   -- padding
	--),
	t.Pack('i1'),
	t.Pack('i1')
)


s = t.Pack (
	{ length       = t.Pack('<i2') },
	{ ['type']     = t.Pack('<i2') },
	{ ['@status']  = t.Pack('i1') },
	{ ConsistCount = t.Pack('i1') }
)
sN= t.Pack('i2i2i1i1')

sbits   = t.Pack (
	{isHealthy   = 'r'},  -- is the train set healthy?
	{isZeroSpeed = 'r'},  -- is the train set stationary?
	{isMaintMode = 'r'},  -- is the train set in maint mode?
	{isPassReq   = 'r'},  -- is a passenger request active?
	{isFileForDl = 'r'},  -- is a file waiting for download (RTDM ready)
	{isVmdsConnd = 'r'},  -- is the connection to the vmds lost?
	{isShopMode  = 'r'},  -- is the train set in shop mode?
	{paddingBit  = 'r'}   -- padding
)


tr = t.Pack (
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

--for k,v in pairs(tr) do print( k, v, v(b) ) end
--for k,v in pairs(tr.status) do print( k, v, v(b) ) end

