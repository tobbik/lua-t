#!../out/bin/lua

---
-- \file    bufConstruct.c
-- \brief   basic tests of xt.Buffer Constructor
local t = require("xt").Test()

t:setUp = function(self)
	-- assemble the test strings
	self.n       = 255
	local sH,sB  = {},{} --string Hex and string Binary
	for i=0,self.n do
		sT[i+1] = string.format('%02X ', i)
		sT[i+1] = string.char(i)
	end
	self.strBin = table.concat (sB, '')
	self.strHex = table.concat (sH, '')
	-- create buffer of 255 bytes len and fill with numeric byte sized values
	self.buf8   = xt.Buffer(n)
	for i=0,n do
		b:write8(i, i)
	end
	-- create a buffer from a string
	self.bufStr  = xt.Buffer(self.strBin)
end

t:testSameContent = function(self)
	self:equal (self.buf8:toHex(), self.bufS:toHex())
	self:equal (self.buf8:toHex(), self.sH)
	print ('buffer conten Hexadecimal representtion is not was expected')
end

t:testSameceLength = function(self)
	self:equal (#self.buf8, self.buf8.length())
	self:equal (#self.buf8, #self.bufS)
	self:equal (#self:buf8, self.n)
	print ('buffer conten Hexadecimal representtion is not was expected')
end


t:run()
