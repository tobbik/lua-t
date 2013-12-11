#!../out/bin/lua

---
-- \file    bufConstruct.c
-- \brief   basic tests of xt.Buffer Constructor
local xt = require ('xt')
--local t = xt.Test('Test basic xt.Buffer functionality')
t = xt.Test('Test basic xt.Buffer functionality')

t.setUp = function(self)
	-- assemble the test strings
	self.n       = 10
	local sH,sB  = {},{} --string Hex and string Binary
	for i=0,self.n do
		sH[i+1] = string.format('%02X ', i)
		sB[i+1] = string.char(i)
	end
	self.strBin = table.concat (sB, '')
	self.strHex = table.concat (sH, '')
	-- create buffer of 255 bytes len and fill with numeric byte sized values
	self.buf8   = xt.Buffer(self.n)
	for i=0,self.n do
		self.buf8:write8(i, i)
	end
	-- create a buffer from a string
	self.bufStr  = xt.Buffer(self.n, self.strBin)
	print ('setup finished');
end

t.test_SameContent = function(self)
	-- #DESC: Check that all buffers have the same content
	self._equal (self.buf8:toHex(), self.bufStr:toHex(), "Hex representation of buffers differs")
	self._equal (self.buf8:toHex(), self.strHex, "Hex representation differs from expected")
end

t.test_SameLength = function(self)
	-- #DESC: Check that all buffers have the same length
	self._equal (#self.buf8, self.buf8:length(), "alpha")
	self._equal (#self.buf8, #self.bufStr, "beta")
	self._equal (#self.buf8, self.n, "gamma")
end

t.test_SameFields = function(self)
	-- #DESC: Check that all buffer fields have the same content
	self._equal (self.buf8:read16(12), self.bufStr:read16(12), "alpha")
end

t.test_dummyStupid = function(self)
	-- #SKIP: Because it would fail anyways
	self._equal (14, 12,"No Sir!")
end


t.test_errno = function(self)
	-- #TODO:
	self.sock = xt.Socket.bind('TCP', '10.20.30.40',55)
	self._equal( self.sock, 'blah')
end

t.tearDown = function(self)
	self.n       = nil
	self.strBin  = nil
	self.strHex  = nil
	-- create buffer of 255 bytes len and fill with numeric byte sized values
	self.buf8    = nil
	self.bufStr  = nil
end

--t:run()
