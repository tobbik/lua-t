#!../out/bin/lua

---
-- \file    bufConstruct.c
-- \brief   basic tests of xt.Buffer Constructor
local t = require("xt").Test('Test basic xt.Buffer functionality')

t:setUp = function("Initiate 2 Buffers, one from string one filled manually")
	-- assemble the test strings
	self.n       = 255
	local sH,sB  = {},{} --string Hex and string Binary
	for i=0,self.n do
		sH[i+1] = string.format('%02X ', i)
		sB[i+1] = string.char(i)
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

t:test1_SameContent = function('Test the equality of the buffers')
	self:_equal (self.buf8:toHex(), self.bufS:toHex())
	self:_equal (self.buf8:toHex(), self.sH)
	print ('buffer content Hexadecimal representation is not was expected')
end

t:test2_SameLength = function('Buffers have the same length and are equal to string length')
	self:_equal (#self.buf8, self.buf8.length())
	self:_equal (#self.buf8, #self.bufS)
	self:_equal (#self:buf8, self.n)
	print ('buffer conten Hexadecimal representtion is not was expected')
end

t:tearDown = function("TearDown")
	self.n       = nil
	self.strBin  = nil
	self.strHex  = nil
	-- create buffer of 255 bytes len and fill with numeric byte sized values
	self.buf8    = nil
	self.bufStr  = nil
end
