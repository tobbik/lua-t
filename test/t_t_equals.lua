#!../out/bin/lua

---
-- \file   test/t_t_equals.lua
-- \brief   basic tests to show 
local   Test = require( 't.Test' )
local   Time = require( 't.Time' )
local   e    = require( 't' ).equals
local   rep  = string.rep

local tc = {
	test_EqNumbers = function( self )
		Test.Case.describe( 'Test for equality of numeric values' )
		local a,b,c = 4,4,5
		assert(     a == a, "num == num should be true" )
		assert(     e(a,a), "Simple equals(num,num) should be true" )
		assert(     a == b, "num == num should be true" )
		assert(     e(a,b), "Simple equals(num,num) should be true" )
		assert(     a ~= c, "num ~= num should be true" )
		assert( not e(a,c), "Simple equals(num,num) should be false" )
	end,

	test_EqStrings = function( self )
		Test.Case.describe( 'Test for equality of string values' )
		local s,n     = "sdGsrTjw rHbtaEdHy ty34qw 5y246j HPgh p7y7 ", 10000
		local a,b,c,d = rep( s, n ), rep( s, n ), rep( s, n ) .. 's', rep( s, n ) .. '-' 
		assert(     a == a, "str == str should be true" )
		assert(     e(a,a), "Simple equals(str,str) should be true" )
		assert(     a == b, "str == str` should be true" )
		assert(     e(a,b), "Simple equals(str,str`) should be true" )
		-- one string longer
		assert(     a ~= c, "str ~= strX should be true" )
		assert( not e(a,c), "Simple equals(str,strX) should be false" )
		-- last character
		assert(     c ~= d, "str ~= strX should be true" )
		assert( not e(c,d), "Simple equals(str,strX) should be false" )
	end,

	test_EqTable = function( self )
		Test.Case.describe( 'table comparison' )
		assert( e({1,2,3,4,5,6}, {1,2,3,4,5,6}), "{a,b} should equal {a,b}" )
		assert( e({'a','a','a'}, {'a','a','a'}), "{'a','a'} should equal {'a','a'}" )
		assert( e({a=1,b=2,c='x'}, {a=1,b=2,c='x'}), "{a=1} should equal {a=1}" )
		assert( e({a=1,b=2,3,4,5}, {a=1,b=2,3,4,5}), "{a=1,2} should equal {a=1,2}" )
	end,

	test_EqTableNot = function( self )
		Test.Case.describe( 'table INequality comparison' )
		assert( not e({1,2,3,4,5,6}, {1,2,3,4,5,7}), "{a,b} should not equal {a,c}" )
		assert( not e({'a','a','a'}, {'a','a','a','a'}), "{'a'} should not equal {'a','a'}" )
		assert( not e({'a','a','a'}, {'a','a','b'}), "{'a','a'} should not equal {'a','b'}" )
		assert( not e({a=1,b=2,c='x'}, {a=1,b=4,c='x'}), "{a=1} should not equal {a=2}" )
		assert( not e({a=1,b=2,3,4,5}, {a=1,b=2,3,4,4}), "{a=1,2,3} should equal {a=1,2,2}" )
	end,

	test_EqTableRef = function( self )
		Test.Case.describe( "Simple Table Reference and Table Value comparison" )
		local k = {x=1,y=2,z={a=1,b=true,c='string'}}
		local h = k
		local l = {x=1,y=2,z={a=1,b=true,c='string'}}
		assert( k==h, "Table reference comparison should be true" )
		assert( k~=l, "Table reference comparison should be false" )
		assert( e( k, h ), "Table reference equality should be true" )
		assert( e( k, l ), "Table value     equality should be true" )
	end,

	test_EqTableDeep = function( self )
		Test.Case.describe( 'Deep table comparison equal' )
		local k = {x=1,y=2,z={a=1,b=true,c='string' },1,2,3,4,5,6}
		local h = {x=1,y=2,z={a=1,b=true,c='string' },1,2,3,4,5,6}
		local l = {x=1,y=2,z={a=1,b=true,c='stringy'},1,2,3,4,5,6}
		assert(     e( k, h ), "Deep table comparison equal" )
		assert( not e( k, l ), "Deep table comparison not equal" )
	end,

	test_EqTableLength = function( self )
		Test.Case.describe( 'Ensure ALL equality' )
		Test.Case.todo( 'Make sure recursive runs both ways' )
		local k = {x=1,y=2,z=4,a=1,b=true,c='string'}
		local h = {x=1,y=2,z=4,a=1,b=true,c='string',d=5}
		assert( not e( k, h ), "Make sure subsets not equal" )
	end,

	test_EqTableVeryDeep = function( self )
		Test.Case.describe( 'Very Deep table comparison equal' )
		local k = {1,{2,{3,{4,{5,{6,{7,{8,x='z',{9,10}}}}}}}}}
		local h = {1,{2,{3,{4,{5,{6,{7,{8,x='z',{9,10}}}}}}}}}
		local l = {1,{2,{3,{4,{5,{6,{7,{8,x='z',{9,11}}}}}}}}}
		assert(     e( k, h ), "Very Deep table comparison equal" )
		assert( not e( k, l ), "Very Deep table comparison not equal" )
	end,

	test_EqByMetaFunction = function(self)
		Test.Case.describe( 'Test for equality via metatable.__eq' )
		local mt  = { __eq = function (o1,o2) return o1.y==o2.y end}
		local k,h = setmetatable( {x=3,y=2,z=1,1,2}, mt ),
						setmetatable( {x=1,y=2,z=3,a=0}, mt )
		assert( k==h, "Metafunction __eq compares sloppy")
		assert( e(k,h), "equals() handles equality via Metafunction __eq")
	end,

	test_EqByMetaFunctionNot = function(self)
		Test.Case.describe( 'Test for INequality via metatable.__eq' )
		local mt  = { __eq = function (o1,o2) return o1.y~=o2.y end} -- compares ~= 
		local k,h = setmetatable( {x=1,y=2,z=3,a=0,1,2}, mt ),
						setmetatable( {x=1,y=2,z=3,a=0,1,2}, mt )
		assert( k~=h, "Metafunction __eq compares sloppy")
		assert( not e(k,h), "equals() handles INequality via Metafunction __eq")
	end,

	test_EqUserdata = function( self )
		Test.Case.describe( 'Test for equality of userdata' )
		local a,b,c = Time(300), Time(300), Time(301)
		assert(     a == a, "ud == ud should be true" )
		assert(     e(a,a), "Simple equals(ud,ud) should be true" )
		assert(     a == b, "ud == ud' should be true" )
		assert(     e(a,b), "Simple equals(ud,ud') should be true" )
		assert(     a ~= c, "udX ~= udY should be true" )
		assert( not e(a,c), "Simple equals(udX,udY) should be false" )
	end,

}

-- The tests in tc will be executed in random order
local t = Test( tc )
t( )
print( t )
