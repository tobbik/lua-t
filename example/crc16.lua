#!../out/bin/lua
local t = require( 't' )

b=t.Buffer( 'ABCDEFGHxx' )
print( b:toHex( ) )

b:write( string.char( 0x22, 0x22) , 8 )
print( b:toHex( ) )

crc1  = t.Encode.Crc(1, true)    --      1 = CRC16
crc2  = t.Encode.Crc(1, false)   --      1 = CRC16
crcA = crc1:calc('ABCDEFGH')
crcB = crc2:calc('ABCDEFGH')
print (string.format('%d  0x%04X', crcA,crcA), #b)
print (string.format('%d  0x%04X', crcB,crcB), #b)
crc1:reset()
crcC = crc1:calc(b, 0, #b-2)
print (string.format('%d  0x%04X', crcC,crcC), #b)


--crc8
crc8  = t.Encode.Crc(0, true)    --      0 = CRC8
crcA8 = crc8:calc('ABCDEFGH')
print (string.format('%d  0x%02X', crcA8,crcA8), #b)
crc8:reset()
crcC8 = crc8:calc(b, 0, #b-2)
print (string.format('%d  0x%02X', crcC8,crcC), #b)
