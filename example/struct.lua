xt=require'xt'


t = xt.Packer.Struct (
	{ length   = xt.Packer.Int(2) },
	{ ['type'] = xt.Packer.Int(2) },
	--{ 'status' = xt.Combinator.Struct (
	-- payload  // Train
	-- boolean flags -> status
	--7     is_healthy          -- is the train set healthy?
	--6     is_zero_speed       -- is the train set stationary?
	--5     is_maint            -- is the train set in maint mode?
	--4     is_pcs              -- is a passenger request active?
	--3     is_file             -- is a file waiting for download (RTDM ready)
	--2     vmds_not_connected  -- is the connection to the vmds lost?
	--1     is_shop_mode        -- is the train set in shop mode?
	--0     is_pad0             -- padding
	-- {'isHealthy'   = xt.Packer.Bit (1)},
	-- {'isZeroSpeed' = xt.Packer.Bit (1)},
	-- {'isMaintMode' = xt.Packer.Bit (1)},
	-- {'isPassReq'   = xt.Packer.Bit (1)},
	-- {'isFileForDl' = xt.Packer.Bit (1)},
	-- {'isVmdsConnd' = xt.Packer.Bit (1)},
	-- {'isShopMode'  = xt.Packer.Bit (1)},
	-- {'padding'     = xt.Packer.Bit (1)}
	--)},
	{['@status']    = xt.Packer.Int(1)},
	{ConsistCount   = xt.Packer.Int(1)}
)
b=xt.Buffer( 'ABCDEFGH' )
t( b, 0 )

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
--]]
