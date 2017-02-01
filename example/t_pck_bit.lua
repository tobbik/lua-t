T      = require't'
Pack   = T.Pack
Buffer = T.Buffer
rep    = string.rep

--     0     1     2     3     4     5     6     7
--  0000  0001  0010  0011  0100  0101  0110  0111
--     8     9     A     B     C     D     E     F
--  1000  1001  1010  1011  1100  1101  1110  1111

-- idx:    1      2    3   4   5   6   7   8   9  10               11121314151617
-- dec:   -7     85   -4  -3  -2  -1   0   1   2   3            36459 t f 0 1 0 -11
-- bin: 1001 1010101 100 101 110 111 000 001 010 011 111000110010100  1 0 0 1 0 1
-- bin: 10011010 10110010  11101110   00001010   01111100 01100101 00100101
--b  = Buffer( string.char( 0x95, 0x65, 0xDC, 0x14, 0xD9 ) )
b  = Buffer( string.char( 0x9A, 0xB2, 0xEE, 0x0A, 0x7C, 0x65, 0x25 ) )

sQ = Pack( 'r4R7r3r3r3r3r3r3r3r3r15vvR1R1r1r1' )

sX = Pack(
	  { signed4bit   = 'r4'  }
	, { unsigned6bit = 'R7'  }
	, { unsigned3_1  = 'r3'  }
	, { unsigned3_2  = 'r3'  }
	, { unsigned3_3  = 'r3'  }
	, { unsigned3_4  = 'r3'  }
	, { unsigned3_5  = 'r3'  }
	, { unsigned3_6  = 'r3'  }
	, { unsigned3_7  = 'r3'  }
	, { unsigned3_8  = 'r3'  }
	, { unsigned4bit = 'r15' }
	, { singleBool1  = 'v'   }
	, { singleBool2  = 'v'   }
	, { singleBool2  = 'R1'  }
	, { singleBool2  = 'R1'  }
	, { singleBool2  = 'r1'  }
	, { singleBool2  = 'r1'  }
)

print( b:toHex( ) )
print( b:toBin( ) )

print( "Sequence sQ:", #sQ, sQ )
print( "-------------------------------------------------------------")
--print( "Sequence sX:", #sX, sX )
--print( "Sequence sI:", #sI, sI )
--for i,v in pairs(sQ) do print(i,'', v(b), v) end
for i=1,#sQ do local f = sQ[i]; print(i,f,'', f(b)) end
--for i,v in pairs(sQ) do print(i,'', sQ[i](b), v(b), sQ[i], v) end
--for i,v in pairs(sX) do print(i,    sX[i](b), v(b), sX[i], v) end

--[[
print( "Sequence s1:", #s1 , s1 )
print( s1[1], s1[1](b) )
print( s1[2], s1[2](b) )
--for k,v in pairs(s1[3]) do print( k, v, v(b) ) end
for i=1,#s1[3] do print( s1[3][i], s1[3][i](b) ) end
print( s1[4], s1[4](b) )


--print( "Sequence Iterator" );
--p_res( s(b) )
print( "Struct  Iterator" );
p_res( s1(b) )

--]]
