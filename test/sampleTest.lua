#!../out/bin/lua

---
-- \file    sampleTest
-- \brief   basic tests to show xt.Test
local   T = require ('xt').Test
t=T('Test some simple stuff')


t.setUp = function(self)
	self.a       = 10
	self.b       = 20
	self.c       = 30
	self.s1      = "This is a String"
	self.s11     = "This is a String"
	self.s2      = "This is anonther String"
end


t.test_EqNumbers = function(self)
	-- #DESC: Test for equality of numeric values
	self._eq (self.b, self.a*2, "Simple Multiplication")
end


t.test_EqNumbersNot = function(self)
	-- #DESC: Test for non equality of numeric values
	self._eq_not (self.b, self.a, "Simple Multiplication")
end


t.test_EqStrings = function(self)
	-- #DESC: Test for equality of String values
	self._eq (self.s1, self.s11, "Same String")
end


t.test_EqStringRef = function(self)
	-- #DESC: Test for equality of String references
	local s = self.s1
	self._eq (self.s1, s, "Same String")
end


t.test_EqStringRefNot = function(self)
	-- #DESC: Test for equality of String references
	local s = self.s1
	s = 'nonsense'
	self._eq_not (self.s1, s, "Same String")
end


t.test_EqTableRef = function(self)
	-- #DESC: Table reference equalty
	local k = {x=1,y=2,z={a=1,b=true,c='string'}}
	local h = k
	self._eq (k,h, "Table reference comparison")
end


t.test_EqTable = function(self)
	-- #DESC: Deep table comparison 
	local k = {x=1,y=2,z={a=1,b=true,c='string'}}
	local h = {x=1,y=2,z={a=1,b=true,c='string'}}
	self._eq (k,h, "Deep table comparison")
end


t.test_EqTableNot = function(self)
	-- #DESC: Deep table comparison (not)
	local k = {x=1,y=2,z={a=1,b=true,c='stringy'}}
	local h = {x=1,y=2,z={a=1,b=true,c='string'}}
	self._eq_not (k,h, "Deep table comparison")
end


t.test_EqMeta = function(self)
	-- #DESC: Test for equality of metatable.__eq
	local mt= {__eq = function (l1,l2) return l1.y == l2.y end}
	local k,h = setmetatable({x=1,y=2,z=3},mt),
	            setmetatable({x=1,y=2,z=3},mt)
	self._eq (k ,h, "Y values of table must be equal")
end


t.test_EqMetaNot = function(self)
	-- #DESC: Test for equality of metatable.__eq
	local mt= {__eq = function (l1,l2) return l1.y ~= l2.y end}
	local k,h = setmetatable({x=1,y=2,z=3},mt),
	            setmetatable({x=1,y=2,z=3},mt)
	self._eq_not (k ,h, "Y values of table must be equal")
end


t.test_LesserThan = function(self)
	-- #DESC: Check that all buffers have the same length
	self._lt (self.b, self.a*2+1, "Simple Multiplication and addition")
end


t.test_busy = function(self)
	-- #DESC: do a dummy loop to eat time
	self.num = 12345678
	for i=1,2*self.num do
		self.num = math.ceil ((self.num+i)%256)
	end
	self._lt (self.num,256,"Modulo shall never exceed its operand ")
end


t.tearDown = function(self)
end

--t:run()
