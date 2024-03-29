---
-- \file      test/t_tbl.lua
-- \brief     Test for extended Table functions  (excluding equals; see -- test/t_tbl_equals.lua)
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Test   = require( "t.Test" )
local Table  = require( "t.Table" )
local Rtvg   = require('t').require( 'rtvg' )

-- \param  n  true if check for existent key else assert non existence
local checkElm = function( t, e, n )
	for i=1,#e do
		assert( (n and t[ e[i] ] ~= nil) or (not n and t[ e[i] ] == nil) ,
		   "Table element " .. e[i] .. " should "..
			(n and '' or 'NOT') .." exist" )
	end
end
local cnt = Table.count
local fnd = Table.find

return {
	-- static members; no need for beforeAll
	len  = math.random( 700, 2000 ),
	rtvg = Rtvg( ),

	-- -----------------------------------------------------------------------
	-- Tests Cases
	-- -----------------------------------------------------------------------

	mapTostring = function( self )
		Test.describe( "Table.map( table, lower )" )
		local words, lower, mixed = { }, 0, 0
		for x = 1, math.random( 500, 1000 ) do
			local w = self.rtvg:getString( )
			if x%2==0 then
				w = w:lower()
			end
			table.insert( words, w )
			if w ~= w:lower( ) then
				mixed = mixed + 1
			else
				lower = lower + 1
			end
		end
		assert( lower > 0, "More than one only lowercase word exepcted" )
		local res = Table.map( words, string.lower )
		for k,v in pairs( res ) do
			assert( v:lower() == v, ("Value should be lowercased, but was <%s>"):format( v ) )
		end
	end,

	mapArthmetic = function( self )
		Test.describe( "Table.map( table, mathFunction )" )
		local nums, sum, sumc = { }, 0, 0
		for x = 1, math.random( 500, 1000 ) do
			local i = self.rtvg:getInteger( )
			table.insert( nums, i )
			sum = sum + i
		end
		local res = Table.map( nums, function(x) return 2*x end )
		for x = 1,#res do sumc = sumc+res[x] end
		assert( sum * 2 == sumc,
			("Sum should be %d doubled but was %d"):format( sum*2, sumc ) )
	end,

	foreachTostring = function( self )
		Test.describe( "Table.foreach( table, tostring )" )
		local ary = self.rtvg:getVals( self.len )
		local typeNames = {}
		for k,v in pairs( ary ) do
			local t = type( v )
			if not typeNames[ t ] then typeNames[ t ] = true end
		end
		assert( Table.length( typeNames) > 1, "More than one type in array expected" )
		Table.foreach( ary, tostring )
		for k,v in pairs( ary ) do
			assert( "string" == type( v ), ("Type should be <string>, but was <%s>"):format( type(v) ) )
		end
	end,

	forEach = function( self )
		Test.describe( "Table.map( table, mathFunction )" )
		local nums, sum, sumc = { }, 0, 0
		for x = 1, math.random( 500, 1000 ) do
			local i = self.rtvg:getInteger( )
			table.insert( nums, i )
			sum = sum + i
		end
		Table.foreach( nums, function(x) return 2*x end )
		for x = 1,#nums do sumc = sumc+nums[x] end
		assert( sum * 2 == sumc,
			("Sum should be %d doubled but was %d"):format( (sum*2), sumc ) )
	end,

	mergeUnion = function( self )
		Test.describe( "Table.merge( tblA, tblB, true ) -> Union" )
		local tblA = {   a=1,  b=2,  c=3,  d=4,  e=5,  f=6, g=7, h=8, i=9,j=10, k=11, l=12, m=13 }
		local tblB = { k=110,l=120,m=130,n=140,o=150,p=160,q=17,r=18,s=19,t=20, u=21, v=22, w=23 }
		local resU = {   a=1,  b=2,  c=3,  d=4,  e=5,  f=6, g=7, h=8, i=9,j=10,k=110,l=120,m=130,
		                                 n=140,o=150,p=160,q=17,r=18,s=19,t=20, u=21, v=22, w=23 }
		local u    = Table.merge( tblA, tblB, true )
		assert( cnt( resU ) == cnt( u ),
			("#Union should be: %d but was %d"):format( cnt(resU), cnt(u) ) )
		checkElm( u, {'a','b','c','d','e','f','g','h','i','j','k','l',
		              'm', 'n','o','p','q','r','s','t','u','v','w' }, true )
		checkElm( u, {'x','y','z'}, false )
		-- make sure tablB overwrites tblA values
		assert( Table.equals( u, resU ), "Union result didn't match expected values" )
	end,

	mergeIntersect = function( self )
		Test.describe( "Table.merge( tblA, tblB, false ) -> Intersection" )
		local tblA = {  a=1,  b=2,  c=3,  d=4,  e=5,  f=6,  g=7,  h=8,  i=9, j=10, k=11, l=12, m=13 }
		local tblB = { g=70, h=80, i=90,j=100,k=110,l=120,m=130,n=140,o=150,p=160,q=170,r=180,s=190 }
		local resI = { g=70, h=80, i=90,j=100,k=110,l=120,m=130 }
		local i    = Table.merge( tblA, tblB, false )
		assert( cnt( resI )  == cnt( i ),
			("#Intersection should be: %d but was %d"):format( cnt(resI), cnt(i) ) )
		checkElm( i, {'g','h','i','j','k','l','m'}, true )
		checkElm( i, {'a','b','c','d','e','f','n','o','p','q','r','s','t','u','v','w' }, false )
		-- make sure tablB overwrites tblA values
		assert( Table.equals( i, resI ), "Intersection result didn't match expected values" )
	end,

	complement = function( self )
		Test.describe( "Table.complement( tblA, tblB, false ) -> Complement" )
		local tblA = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9,j=10,k=11,l=12,m=13 }
		local tblB = { g=7, h=8, i=9,j=10,k=11,l=12,m=13,n=14,o=15,p=16,q=17,r=18,s=19 }
		local c    = Table.complement( tblA, tblB, false )
		assert( cnt( tblA ) - 7  == cnt( c ),
			("#Complement should be: %d but was %d"):format( cnt(tblA)-7, cnt(c) ) )
		checkElm( c, {'a','b','c','d','e','f'}, true )
		checkElm( c, {'g','h','i','j','k','l','m'}, false )
	end,

	complementSymmetricDifference = function( self )
		Test.describe( "Table.complement( tblA, tblB, true ) -> Symmetric Diff" )
		local tblA = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9, j=10,k=11,l=12,m=13 }
		local tblB = { f=6, g=7, h=8, i=9, j=10,k=11,l=12,m=13,n=14,o=15,p=16,q=17,r=18 }
		local c    = Table.complement( tblA, tblB, true )
		assert( cnt( tblA ) + cnt( tblB ) - 8 - 8 == cnt( c ),
			("#Complement should be: %d but was %d"):format( cnt(tblA) + cnt(tblB)-8-8, cnt(c) ) )
		checkElm( c, {'a','b','c','d','e','n','o','p','q','r'}, true )
		checkElm( c, {'g','h','i','j','k','l','m'}, false )
	end,

	containsSubset = function( self )
		Test.describe( "Table.contains( tblA, tblB ) -> B subset of A" )
		local tblA = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9, j=10,k=11,l=12,m=13 }
		local tblS = { b=2, d=4, f=6,j=10,l=12 }
		local contains = Table.contains( tblA, tblS )
		assert( contains, "tblA should contain tblS" )
		assert( Table.contains( tblA, tblS ) == Table.contains( tblA, tblS, false),
				"contains with false 3rd arguument should still test for subset" )
	end,

	containsNotSubset = function( self )
		Test.describe( "Table.contains( tblA, tblB ) -> B is not subset of A" )
		local tblA = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9, j=10,k=11,l=12,m=13 }
		local tblS = { b=2, d=4, f=6,j=10,l=12,    r=18 }
		local contains = Table.contains( tblA, tblS )
		assert( not contains, "tblA should contain tblS" )
		assert( Table.contains( tblA, tblS ) == Table.contains( tblA, tblS, false),
				"contains with false 3rd arguument should still test for subset" )
	end,

	disjointSets = function( self )
		Test.describe( "Table.contains( tblA, tblB, true ) -> Are disjoint" )
		local tblA = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9,j=10,k=11,l=12,m=13 }
		local tblB = {               n=14,o=15,p=16,q=17,r=18,s=19,t=20,u=21,v=22,w=23 }
		local disjoint  = Table.contains( tblA, tblB, true )
		assert( disjoint, "tblA and tablB should be disjoint" )
	end,

	disjointSetsNot = function( self )
		Test.describe( "Table.contains( tblA, tblB, true ) -> Are NOT disjoint" )
		local tblA = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9,j=10,k=11,l=12,m=13 }
		local tblB = {          m=13,n=14,o=15,p=16,q=17,r=18,s=19,t=20,u=21,v=22,w=23 }
		local disjoint  = Table.contains( tblA, tblB, true )
		assert( not disjoint, "tblA and tablB should not be disjoint" )
	end,

	findValue = function( self )
		Test.describe( "Table.find( tbl, val, idx ) -> find value in table" )
		local tbl = {
			load, 400, 88.88888, 'a string', {4,8,12,16}, true, 'a string', [false] = 'special',
			fnc=print, int=400, float=88.88887, str='another string', tbl={4,8,12,17}, bool=false}
		assert( 1 == fnd( tbl, load ), "`load` should be at index 1" )
		assert( 2 == fnd( tbl, 400 ), "400 should be at index 2" )
		assert( 7 == fnd( tbl, 'a string', 5 ), "'a string' past index 4 should be at index 7" )
		-- passing an index to find limits the search to the numeric part
		assert( not fnd( tbl, 400, 3 ), "400 past index 2 should't be found" )
		assert( 'bool' == fnd( tbl, false ), "key for false should be bool" )
		local f = fnd( tbl, special )
		assert( not fnd( tbl, 'special' ), "returning key false should be evaluated as boolean" )
		assert( 'boolean' == type( fnd( tbl, 'special' ) ), "key should be `false` not `nil`" )
		assert( not fnd( tbl, 'not exist' ), "key should be `nil` not `false`" )
		assert( 'nil' == type( fnd( tbl, 'not exist' ) ), "key should be `nil` not `false`" )
	end,

	keys = function( self )
		Test.describe( "Table.keys( tbl ) -> array of keys" )
		local tbl  = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9,j=10,k=11,l=12,m=13 }
		local keys = Table.keys( tbl );
		assert( #keys == cnt(tbl), "#keys should be: " .. cnt(tbl) )
		for i=1,#keys do assert( tbl[ keys[i] ], keys[i].." key should exist in table" ) end
	end,

	values = function( self )
		Test.describe( "Table.keys( tbl ) -> array of keys" )
		local tbl  = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, i=9,j=10,k=11,l=12,m=13 }
		local vals = Table.values( tbl );
		assert( #vals == cnt(tbl), "#values should be: " .. cnt(tbl) )
		for i=1,#vals do assert( fnd(tbl, vals[i]), vals[i].." value should exist in table" ) end
	end,

	count = function( self )
		Test.describe( "Test various tables count" )
		local t1   = {a='a',b='b'}
		local t2   = {1,2,3,4,5}
		local t3   = {a='a',b='b',1,2,3,4,5}
		assert( 0 == cnt( {} ), "Length of emty table should be 0" )
		assert( 2 == cnt( t1 ), "Length of hashtable should be 2" )
		assert( 5 == cnt( t2 ), "Length of numeric table should be 5" )
		assert( 7 == cnt( t3 ), "Length of mixed keys table should be 7" )
	end,

	clone = function( self )
		Test.describe( "Table.clone( tbl ) -> clone table" )
		local tbl   = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8, 9,10,11,12,13,14 }
		local clone = Table.clone( tbl );
		assert( #tbl == #clone, "#clone should be: " .. #tbl .. " but was: " .. #clone )
		assert( cnt(tbl) == cnt(clone), "cnt(clone) should be: " .. cnt(tbl) ..
		         " but was: " .. cnt(clone) )
		for k,v in pairs(clone) do assert( v == tbl[k], k.." key should exist in original" ) end
		for k,v in pairs(tbl) do assert( v == clone[k], k.." key should exist in clone" ) end
	end,

	deepClone = function( self )
		Test.describe( "Table.clone( tbl, true ) -> deep clone table" )
		local tbl = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8,  9,10,11,12,13,14 }
		tbl.tbl   = { k=11,l=12,m=13,n=14,o=15,p=16,q=17,r=18, 19,20,21,22,23,24 }
		local clone = Table.clone( tbl, true );
		assert( #tbl == #clone, "#clone should be: " .. #tbl .. " but was: " .. #clone )
		assert( #tbl.tbl == #clone.tbl, "#clone should be: " .. #tbl .. " but was: " .. #clone )
		assert( cnt(tbl) == cnt(clone), "cnt(clone) should be: " .. cnt(tbl) ..
		         " but was: " .. cnt(clone) )
		assert( cnt(tbl.tbl) == cnt(clone.tbl), "cnt(clone) should be: " .. cnt(tbl.tbl) ..
		         " but was: " .. cnt(clone.tbl) )
		assert( Table.equals( tbl, clone), "Orginal and deep clone should be equal" )
	end,

	countAndIsempty = function( self )
		Test.describe( "Test various tables for emptyness and count" )
		local t1   = {a='a'}
		local t2   = {1,2,3,4,5}; t2[1]=nil; t2[2] = nil; t2[5]=nil
		assert( 0 == #t1     , "Length of hashtable should be 0" )
		-- behavior for #t2 has changed from Lua-5.3 to 5.4; used to be 0 but now
		-- seems highest original index
		assert( 5 ~= #t2     , "#table with gaps shouldn't be original length" )
		assert( 1 == cnt(t1) , "Count of hashtable should be 1" )
		assert( 2 == cnt(t2) , "Count of tables with gaps should be 2" )
		assert( not Table.isempty( t1 ), "Hashtable should not be empty" )
		assert( not Table.isempty( t2 ), "Table with gaps should not be empty" )
		assert( Table.isempty( { } ), "Empty Table should be empty" )
	end,

}
