#!../out/bin/lua
local xt = require("xt")

b=xt.Buffer(10, 'ABCDEFGHxx')
print(b:toHex())

b:write16(8, 0x2222)
print (b:toHex())

crc1  = xt.Encode.Crc(0, true)    --      0 = CRC16
crc2  = xt.Encode.Crc(0, false)   --      0 = CRC16
crcA = crc1:calc('ABCDEFGH')
crcB = crc2:calc('ABCDEFGH')
print (string.format('%d  0x%04X', crcA,crcA), #b)
print (string.format('%d  0x%04X', crcB,crcB), #b)
crc1:reset()
crcC = crc1:calc(b, 0, #b-2)
print (string.format('%d  0x%04X', crcC,crcC), #b)
