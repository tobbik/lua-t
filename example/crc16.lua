#!../out/bin/lua
local xt = require("xt")

b=xt.Buffer(10, 'ABCDEFGHxx')
print(b:toHex())

--b:write16(8, 0x2222)
--print (b:toHex())
--crcA = b:getCrc16(true , 0, #b-2)
--crcB = b:getCrc16(false, 0, #b-2)
--print (string.format('%d  0x%04X', crcA,crcA), #b)
--print (string.format('%d  0x%04X', crcB,crcB), #b)

--b:write16(8, b:getCrc16(true , 0, #b-2))
--print (b:toHex())
--b:write16(8, b:getCrc16(false , 0, #b-2))
--print (b:toHex())



--================ NEX CRC handling
crc  = xt.Encode.Crc(0)   --      0 = CRC16
crcC = crc:calc('ABCDEFGH')
print (string.format('%d  0x%04X', crcC,crcC), #b)
