#!../out/bin/lua
xt=require'xt'


s = xt.Packer.Sequence (
	xt.Packer.Bit( 1 ),  -- is the train set healthy?
	xt.Packer.Bit( 1 ),  -- is the train set stationary?
	xt.Packer.Bit( 1 ),  -- is the train set in maint mode?
	xt.Packer.Bit( 1 ),  -- is a passenger request active?
	xt.Packer.Bit( 1 ),  -- is a file waiting for download (RTDM ready)
	xt.Packer.Bit( 1 ),  -- is the connection to the vmds lost?
	xt.Packer.Bit( 1 ),  -- is the train set in shop mode?
	xt.Packer.Bit( 1 )   -- padding
)

q = xt.Packer.Sequence (
	xt.Packer.Int(2),
	xt.Packer.Int(2),
	xt.Packer.Int(1),
	xt.Packer.Int(1)
)
b=xt.Buffer( 'ABCDEFGH' )
--t( b, 0 )

m=getmetatable(s)
