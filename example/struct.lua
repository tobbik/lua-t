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
	t.Pack(
		t.Pack('v'),  -- is the train set healthy?
		t.Pack('v'),  -- is the train set stationary?
		t.Pack('v'),  -- is the train set in maint mode?
		t.Pack('v'),  -- is a passenger request active?
		t.Pack('v'),  -- is a file waiting for download (RTDM ready)
		t.Pack('v'),  -- is the connection to the vmds lost?
		t.Pack('v'),  -- is the train set in shop mode?
		t.Pack('v')   -- padding
	),
	p,
	p,
	'rrrrrrrr',
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
	{isHealthy   = 'v'},  -- is the train set healthy?
	{isZeroSpeed = 'v'},  -- is the train set stationary?
	{isMaintMode = 'v'},  -- is the train set in maint mode?
	{isPassReq   = 'v'},  -- is a passenger request active?
	{isFileForDl = 'v'},  -- is a file waiting for download (RTDM ready)
	{isVmdsConnd = 'v'},  -- is the connection to the vmds lost?
	{isShopMode  = 'v'},  -- is the train set in shop mode?
	{paddingBit  = 'v'}   -- padding
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

