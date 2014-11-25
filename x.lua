T = require't'

sq = T.Pack.Sequence (
	T.Pack.IntB2,
	T.Pack.IntB2,
	T.Pack.Int1,
	T.Pack.Int1
)

st = T.Pack.Struct (
	{ length       = T.Pack.IntB2 },
	{ ['type']     = T.Pack.IntL2 },
	{ ['@status']  = T.Pack.Int1  },
	{ ConsistCount = T.Pack.Int1  }
)

--st1 = T.Pack.Struct (
--	{ length       = '>i2' },
--	{ ['type']     = '<i2' },
--	{ ['@status']  = 'i1'  },
--	{ ConsistCount = 'i1'  }
--)
--t.Pack('>i2<i2i1i1', 'length', 'type','@status','ConsistCount')



s = 'aBcDeF'
b= T.Buffer(s)
fmt='>i2<i2bb'
print( string.unpack( fmt, s ) )

for k,v in pairs( st(b) ) do print(k,v) end


