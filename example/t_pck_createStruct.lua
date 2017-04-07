#!../out/bin/lua
Pack,Buffer=require't.Pack',require't.Buffer'

-- pack syntax for the format string is based of Lua 5.3 
p = Pack('<i2')                         -- Little endian 2 byt integer
c = Pack('>i2')

ts = Pack(
	p,
	Pack('>i2'),
	Pack('i1'),
	Pack('i1')
)

ss = Pack (
	p,
	p,
	Pack(
		Pack('v'),  -- is the train set healthy?
		Pack('v'),  -- is the train set stationary?
		Pack('v'),  -- is the train set in maint mode?
		Pack('v'),  -- is a passenger request active?
		Pack('v'),  -- is a file waiting for download (RTDM ready)
		Pack('v'),  -- is the connection to the vmds lost?
		Pack('v'),  -- is the train set in shop mode?
		Pack('v')   -- padding
	),
	p,
	p,
	'rrrrrrrr',
	Pack('i1'),
	Pack('i1')
)


s = Pack (
	{ length       = Pack('<i2') },
	{ ['type']     = Pack('<i2') },
	{ ['@status']  = Pack('i1') },
	{ ConsistCount = Pack('i1') }
)
sN= Pack('i2i2i1i1')

sbits   = Pack (
	{isHealthy   = 'v'},  -- is the train set healthy?
	{isZeroSpeed = 'v'},  -- is the train set stationary?
	{isMaintMode = 'v'},  -- is the train set in maint mode?
	{isPassReq   = 'v'},  -- is a passenger request active?
	{isFileForDl = 'v'},  -- is a file waiting for download (RTDM ready)
	{isVmdsConnd = 'v'},  -- is the connection to the vmds lost?
	{isShopMode  = 'v'},  -- is the train set in shop mode?
	{paddingBit  = 'v'}   -- padding
)


tr = Pack (
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

b=Buffer( 'ABCDEFGHIJKLMNOPQRST' )

--for k,v in pairs(tr) do print( k, v, v(b) ) end
--for k,v in pairs(tr.status) do print( k, v, v(b) ) end

