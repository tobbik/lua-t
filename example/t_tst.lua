#!../out/bin/lua

---
-- \file    sampleTest.lua
-- \brief   basic tests to show t.Test
local   Test   = require( 't.Test' )

tc = {
	beforeAll = function( self, done )  -- not necessary for this suite
		print( 'RUNNING beforeAll' )
		done(self)
	end,

	afterAll = function( self, done )  -- not necessary for this suite
		print( 'RUNNING afterAll' )
		done(self)
	end,

	beforeEach = function( self )
		self.a    = 10
		self.b    = 20
		self.c    = 30
		self.s1   = "This is a String"
		self.s11  = "This is a String"
		self.s2   = "This is anonther String"
		print( 'RUNNING beforeEach' )
	end,

	afterEach = function( self )  -- not necessary for this suite
		print( 'RUNNING afterEach' )
	end,

	test_EqNumbers = function( self )
		Test.Case.describe( 'Test for equality of numeric values' )
		Test.Case.skip( 'Numbers are all over the place today...' )
		assert( self.b==self.a*2, "Multiplication equals result" )
	end,

	test_EqNumbersNot = function( self )
		Test.Case.describe( 'Test for non equality of numeric values' )
		assert( self.b~=self.a, "Multiplication does not equal result" )
	end,

	test_EqStrings = function( self )
		Test.Case.describe( 'Test for equality of String values' )
		assert( self.s1==self.s11, "Strings are not equal" )
	end,

	test_EqStringRef = function( self )
		Test.Case.describe( 'Test for equality of String references' )
		local s = self.s1
		assert( self.s1==s, "Same String" )
	end,

	test_EqStringRefNot = function( self )
		Test.Case.describe( 'Test for equality of String references' )
		local s = self.s1
		s = 'nonsense'
		assert( self.s1 ~= s, "Same String" )
	end,

	test_EqTableRef = function( self )
		-- Purposefully no Test.Case.describe() so it uses the function name instead
		local k = {x=1,y=2,z={a=1,b=true,c='string'}}
		local h = k
		assert( k==h, "Table reference comparison" )
	end,

	test_EqTable = function( self )
		Test.Case.describe( 'Deep table comparison' )
		local k = {x=1,y=2,z={a=1,b=true,c='string'}}
		local h = {x=1,y=2,z={a=1,b=true,c='string'}}
		assert( Test.equal( k, h ), "Deep table comparison" )
	end,

	test_EqTableNot = function( self )
		Test.Case.describe( 'Deep table comparison (not equal)' )
		local k = {x=1,y=2,z={a=1,b=true,c='stringy'}}
		local h = {x=1,y=2,z={a=1,b=true,c='string'}}
		assert( not Test.equal( k, h ), "Deep table comparison" )
	end,

	test_ToSkip = function( self )
		Test.Case.describe( 'Test for skipping' )
		Test.Case.skip( '...for fun and profit ...')
		assert( self.b==self.a*2, "Multiplication equals result" )
	end,
}

-- The tests in tc will be executed in random order
t = Test( tc )

-- Test from here on will ALWAYS be executed in the order they are written and
-- AFTER the tests in the table passed to the constructor
t.test_EqTableRecursive = function( self )
	Test.Case.describe( 'Deep table comparison with first table shorter than second' )
	local h = {6,7,8,9,'str',{'A','B','C','D',{'z','y','x'}    },3,4,5,6}
	local k = {6,7,8,9,'str',{'A','B','C','D',{'z','y','x','w'}},3,4,5}
	assert( not Test.equal( h, k ), "Deep table comparison with different table sizes" )
end

t.test_MakeFail = function( self )
	Test.Case.describe('Create an error in code')
	Test.Case.todo( 'Nothing todo here. This test fails to test the test framework' )
	local x = 5
	x = x.a + 5
	assert( x==6 , "This math makes no sense" )
end

t.test_EqTableRevK = function( self )
	Test.Case.describe( 'Deep table comparison with first table different from second' )
	local k = {x=1,y=2,z={a=1,b=true,c='string'},d='not in second'}
	local h = {x=1,y=2,z={a=1,b=true,c='string'}                  }
	assert( Test.equal( h, k ), "Deep table comparison with different table sizes" )
end

t.test_EqMeta = function(self)
	Test.Case.describe( 'Test for equality of metatable.__eq' )
	local mt= {__eq = function (l1,l2) return l1.y == l2.y end}
	local k,h = setmetatable( {x=1,y=2,z=3},mt ),
					setmetatable( {x=1,y=2,z=3},mt )
	assert( k==h, "Y values of table must be equal")
end

t.test_EqMetaNot = function( self )
	Test.Case.describe( 'Test for equality of metatable.__eq' )
	local mt= {__eq = function (l1,l2) return l1.y ~= l2.y end}
	local k,h = setmetatable({x=1,y=2,z=3},mt),
					setmetatable({x=1,y=2,z=3},mt)
	assert( k~=h, "Y values of table must be equal" )
end

t.test_LesserThan = function( self )
	Test.Case.describe( 'Check that all buffers have the same length' )
	assert( self.b < self.a*2+1, "Simple Multiplication and addition" )
end

t.test_busy = function( self )
	Test.Case.describe( 'do a dummy loop to eat time' )
	self.num = 1234567
	for i=1,2*self.num do
		self.num = math.ceil( (self.num+i)%256 )
	end
	assert( self.num<256, "Modulo shall never exceed its operand" )
end

t( )
print(t)
